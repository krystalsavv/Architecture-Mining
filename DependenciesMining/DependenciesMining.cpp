#include "DependenciesMining.h"
#include "Utilities.h"

#define CLASS_DECL "ClassDecl"
#define STRUCT_DECL "StructDecl"
#define FIELD_DECL "FieldDecl"
#define METHOD_DECL "MethodDecl"
#define METHOD_VAR_OR_ARG "MethodVarOrArg"


using namespace DependenciesMining;

StructuresTable DependenciesMining::structuresTable;

DeclarationMatcher ClassDeclMatcher = anyOf(cxxRecordDecl(isClass()).bind(CLASS_DECL), cxxRecordDecl(isStruct()).bind(STRUCT_DECL));
DeclarationMatcher FieldDeclMatcher = fieldDecl().bind(FIELD_DECL);
DeclarationMatcher MethodDeclMatcher = cxxMethodDecl().bind(METHOD_DECL);
DeclarationMatcher MethodVarMatcher = varDecl().bind(METHOD_VAR_OR_ARG);

// Handle all the Classes and Structs and the Bases
void ClassDeclsCallback::run(const MatchFinder::MatchResult& result) {
	const CXXRecordDecl* d;
	Structure structure;
	if ((d = result.Nodes.getNodeAs<CXXRecordDecl>(CLASS_DECL))) {
		structure.SetStructureType(StructureType::Class);
	}
	else if ((d = result.Nodes.getNodeAs<CXXRecordDecl>(STRUCT_DECL))) {
		structure.SetStructureType(StructureType::Struct);
	}
	else {
		assert(0);
	}

	//if (d->isInStdNamespace() || !d->hasDefinition()) {
	if (!d->hasDefinition()) {
		return;														// ignore std && declarations
	}
	if (d->isImplicit()) {
		return;
	}

	// Templates
	if (d->getDescribedClassTemplate()) {	
		structure.SetStructureType(StructureType::TemplateDefinition);
	} 
	else if (d->getKind() == d->ClassTemplatePartialSpecialization) {
		structure.SetStructureType(StructureType::TemplatePartialSpecialization);

	} 
	else if (d->getKind() == d->ClassTemplateSpecialization) {
		if(d->getTemplateSpecializationKind() == 1)
			structure.SetStructureType(StructureType::TemplateInstatiationSpecialization);
		else 
			structure.SetStructureType(StructureType::TemplateFullSpecialization);
	}else if (d->getKind() == d->TemplateTemplateParm) {
		assert(0);
	}
	else if (d->getKind() == d->TemplateTypeParm) {
		assert(0);
	}

	//std::cout << "Name: " << d->getNameAsString() << "\tQualifiedName: " << d->getQualifiedNameAsString() << "\nMy Name: " << GetFullStructureName(d) << "\n\n";
	auto srcLocation = result.SourceManager->getPresumedLoc(d->getLocation());
	structure.SetTotalLocation(srcLocation.getFilename(), srcLocation.getLine(), srcLocation.getColumn());
	
	auto structID = d->getID(); 
	assert(structID); 
	structure.SetName(GetFullStructureName(d));
	structure.SetID(structID);
	
	// Templates
	//if (d->getKind() == d->ClassTemplateSpecialization || d->getKind() == d->ClassTemplatePartialSpecialization) {
	if (structure.IsTemplateInstatiationSpecialization()){
		// Template parent
		std::string parentName;
		ID_T parentID; 

		//if (structure.IsTemplateInstatiationSpecialization()) {
			auto* parent = d->getTemplateInstantiationPattern();
			parentID = parent->getID();
			assert(parentID); 
			parentName = GetFullStructureName(parent);
		//}
		//else {
		//	parentName = d->getQualifiedNameAsString();				// mporei kai na mhn xreiazetai na orisw parent mia kai to full specialization einai mia diaforetikh class (partial specialization einai san new template)
		//}
		Structure* templateParent = structuresTable.Get(parentID);
		if (!templateParent)
			templateParent = structuresTable.Insert(parentID, parentName);
		structure.SetTemplateParent(templateParent);

		//Template Arguments		
		auto* temp = (ClassTemplateSpecializationDecl*)d;
		for (unsigned i = 0; i < temp->getTemplateArgs().size(); ++i) {
			auto templateArg = temp->getTemplateArgs()[i];
			TemplateArgsVisit(templateArg, [](TemplateArgument templateArg, Structure *structure) {
				RecordDecl* d = nullptr;
				if (templateArg.getKind() == TemplateArgument::Template) {
					d = (RecordDecl*)templateArg.getAsTemplateOrTemplatePattern().getAsTemplateDecl();
				}
				if (d || GetTemplateArgType(templateArg)->isStructureOrClassType()) {
						if (!d)
							d = GetTemplateArgType(templateArg)->getAsCXXRecordDecl();			
						std::string argName = GetFullStructureName(d);
						auto argID = d->getID(); 
						assert(argID); 
						Structure* arg = structuresTable.Get(argID);
						if(arg == nullptr)
							arg = structuresTable.Insert(argID, argName);
						structure->InsertTemplateSpecializationArguments(argID, arg);
					}
				}, &structure);			
		}
	}
	
	// Namespace
	auto* enclosingNamespace = d->getEnclosingNamespaceContext();
	std::string fullEnclosingNamespace = "";
	while (enclosingNamespace->isNamespace()) {
		fullEnclosingNamespace = ((NamespaceDecl*)enclosingNamespace)->getNameAsString() + "::" + fullEnclosingNamespace;
		enclosingNamespace = enclosingNamespace->getParent()->getEnclosingNamespaceContext();
	}
	structure.SetEnclosingNamespace(fullEnclosingNamespace);
	
	// Bases
	for(auto it : d->bases()){
		auto* baseRecord = it.getType()->getAsCXXRecordDecl();	
		if (baseRecord == nullptr)										// otan base einai template or partial specialization Ignored
			continue;
		std::string baseName = GetFullStructureName(baseRecord);
		auto baseID = baseRecord->getID();
		assert(baseID);
		Structure* base = structuresTable.Get(baseID);
		if (!base)
			base = structuresTable.Insert(baseID, baseName);
		structure.InsertBase(baseID, base);
	}

	// Friends 
	for (auto* it : d->friends()) {
		auto* type = it->getFriendType();
		if (type) {																							// Classes
			auto parent = type->getType()->getAsCXXRecordDecl(); 
			auto parentID = parent->getID();
			assert(parentID);
			std::string parentName = GetFullStructureName(type->getType()->getAsCXXRecordDecl()) ;
			Structure* parentStructure = structuresTable.Get(parentID);
			if (!parentStructure) continue;								// templates
			structure.InsertFriend(parentID, parentStructure);
		}
		else {																								// Methods			
			auto* decl = it->getFriendDecl();
			if (decl->getKind() == it->CXXMethod) {
				std::string methodName = GetFullMethodName((CXXMethodDecl*)decl);
				auto* parentClass = ((CXXMethodDecl*)decl)->getParent();
				auto parentClassID = parentClass->getID();
				assert(parentClassID);
				std::string parentName = GetFullStructureName(parentClass);
				Structure* parentStructure = structuresTable.Get(parentClassID);
				if (!parentStructure) continue;
				// meta thn allagh se ids ws keys den krataw info gia to method
				structure.InsertFriend(parentClassID, parentStructure);
			}
		}
	}

	// Members
	if (d->isCXXClassMember()) {
		const auto* parent = d->getParent();
		assert(parent);
		std::string parentName = GetFullStructureName((RecordDecl*)parent);
		auto parentID = ((RecordDecl*)parent)->getID();
		assert(parentID); 
		if (parentName != structure.GetName()) {
			Structure* parentStructure = structuresTable.Get(parentID);
			auto* inst = d->getInstantiatedFromMemberClass();
			structure.SetNestedParent(parentStructure);
			parentStructure->InsertNestedClass(structure.GetID(), structuresTable.Insert(structure.GetID(), structure.GetName()));
		}
	}
	structuresTable.Insert(structure.GetID(), structure);
}

// Hanlde all the Fields in classes/structs
void FeildDeclsCallback::run(const MatchFinder::MatchResult& result) {
	if (const FieldDecl* d = result.Nodes.getNodeAs<FieldDecl>(FIELD_DECL)) {
		if (!isStructureOrStructurePointerType(d->getType()))
			return; 

		const RecordDecl* parent = d->getParent();
		if (parent->isClass() || parent->isStruct()) {
			std::string parentName = GetFullStructureName(parent);
			std::string typeName;
			ID_T parentID = parent->getID(); 
			ID_T typeID = 0; 
			if (d->getType()->isPointerType()) {
				typeName = GetFullStructureName(d->getType()->getPointeeType()->getAsCXXRecordDecl());
				typeID = d->getType()->getPointeeType()->getAsCXXRecordDecl()->getID();
			}
			else {
				typeName = GetFullStructureName(d->getType()->getAsCXXRecordDecl());
				typeID = d->getType()->getAsCXXRecordDecl()->getID();
			}
			assert(parentID); 
			assert(typeID); 
			
			Structure* parentStructure = structuresTable.Get(parentID);
			Structure* typeStructure = structuresTable.Get(typeID);
			if (parentStructure->IsTemplateInstatiationSpecialization())		// insertion speciallization inherite its dependencies from the parent template
				return;
			if (!typeStructure)
				typeStructure = structuresTable.Insert(typeID, typeName);
			
			//llvm::outs() << "Field:  " << d->getName() << "\tQualified Name: " << d->getQualifiedNameAsString() << "\n\tParent: " << parentName << "   " << typeName << "\n";

			auto fieldID = d->getID();
			assert(fieldID);
			Definition field(fieldID, d->getQualifiedNameAsString(), typeStructure);
			auto srcLocation = result.SourceManager->getPresumedLoc(d->getLocation());
			field.SetTotalLocation(srcLocation.getFilename(), srcLocation.getLine(), srcLocation.getColumn());
			parentStructure->InsertField(fieldID, field);	
		} 
	}
}

// Handle all the Methods
void MethodDeclsCallback::run(const MatchFinder::MatchResult& result) {
	if (const CXXMethodDecl* d = result.Nodes.getNodeAs<CXXMethodDecl>(METHOD_DECL)) {
		const RecordDecl* parent = d->getParent();
		std::string parentName = GetFullStructureName(parent);
		auto parentID = parent->getID();
		auto methodID = d->getID(); 
		assert(parentID);
		assert(methodID);

		Structure* parentStructure = structuresTable.Get(parentID);
		//llvm::outs() << "Method:  " << GetFullMethodName(d) << "\n\tParent: " << parentName << "\n\n";
		Method method(methodID, GetFullMethodName(d));
		auto srcLocation = result.SourceManager->getPresumedLoc(d->getLocation());
		method.SetTotalLocation(srcLocation.getFilename(), srcLocation.getLine(), srcLocation.getColumn());

		if (d->getDeclKind() == d->CXXConstructor) {
			method.SetMethodType(MethodType::Constructor);
		}
		else if (d->getDeclKind() == d->CXXDestructor) {
			method.SetMethodType(MethodType::Destructor);
		}
		else if (d->getTemplatedKind()) {
			if (d->getTemplatedKind() == d->TK_FunctionTemplate) {
		////		std::cout << GetFullMethodName(d) << " "<<d->getID() << ": \n";
		//		auto id = d->getIdentifier(); 
		//		
		//		auto funcTempl = d->getDescribedFunctionTemplate(); 
		//		auto params = funcTempl->getTemplateParameters();
		//		for (int i = 0; i < params->size(); i++) {
		//			auto param = params->getParam(i);
		//	
		//			auto kind = param->getKind();
		//			//std::cout << i << " :" << param->getQualifiedNameAsString() << "\n"; 
		//			std::cout << i << " :" << param->getDeclKindName() << "\n";
	
		//		}

				method.SetMethodType(MethodType::TemplateDefinition);
			}
			else if (d->getTemplatedKind() == d->TK_FunctionTemplateSpecialization || d->getTemplatedKind() == d->TK_DependentFunctionTemplateSpecialization) {
				if (d->isTemplateInstantiation()) {
					method.SetMethodType(MethodType::TemplateInstatiationSpecialization);
				}
				else {
					method.SetMethodType(MethodType::TemplateFullSpecialization);
				}
			}
			else if (d->getTemplatedKind() == d->TK_MemberSpecialization) {
				method.SetMethodType(MethodType::UserMethod);		// user method on teplate class
			}
			else {
				std::cout << d->getTemplatedKind() << "\n\n";
				assert(0);
			}
		}
		else {
			method.SetMethodType(MethodType::UserMethod);
		}

		if (parentStructure->IsTemplateInstatiationSpecialization()) {		// insertion speciallization inherite its dependencies from the parent template
			if (method.isUserMethod() || method.isTemplateDefinition()) // || method.isTemplateFullSpecialization()) mporei na kanw special gia authn thn klash
				return;
		}

		//Template
		if (method.isTemplateFullSpecialization() || method.isTemplateInstatiationSpecialization()) {
			//assert(parentStructure->IsTemplate() || parentStructure->IsTemplateFullSpecialization() || parentStructure->IsTemplateInstatiationSpecialization());
		/*	Method* templateParentMethod = nullptr;
			std::string parentMethodName = GetFullMethodName(d);
			size_t start = parentMethodName.find("<");
			size_t end = parentMethodName.find(">");
			parentMethodName.erase(parentMethodName.begin() + start, parentMethodName.begin() + end + 1);
			if (parentStructure->IsTemplateFullSpecialization() || parentStructure->IsTemplateInstatiationSpecialization()) {
				
				templateParentMethod = parentStructure->GetTemplateParent()->GetMethod(parentMethodName);
			}
			else {
				templateParentMethod = parentStructure->GetMethod(parentMethodName);
			}
			assert(templateParentMethod);
			method.SetTemplateParent(templateParentMethod);*/

			//Template Arguments		
			auto args = d->getTemplateSpecializationArgs()->asArray();
			for (auto it : args) {
				TemplateArgsVisit(it, [](TemplateArgument templateArg, Method* method) {
					RecordDecl* d = nullptr;
					if (templateArg.getKind() == TemplateArgument::Template) {
						d = (RecordDecl*)templateArg.getAsTemplateOrTemplatePattern().getAsTemplateDecl();
					}
					else if (templateArg.getKind() == TemplateArgument::Integral) {
						return;
					}
					if (d || GetTemplateArgType(templateArg)->isStructureOrClassType()) {
						if (!d)
							d = GetTemplateArgType(templateArg)->getAsCXXRecordDecl();
						std::string argName = GetFullStructureName(d);
						auto argID = d->getID(); 
						assert(argID);
						Structure* arg = structuresTable.Get(argID);
						if (arg == nullptr)
							arg = structuresTable.Insert(argID, argName);
						method->InsertTemplateSpecializationArguments(argID, arg);
					}
					}, &method);
			}
		}
		else {											// den krataw to return type gia ta specialization (tha ta exei to definition h tha einai tou typou tou pairnei san templateParam)
			//Return
			auto returnType = d->getReturnType();
			if (isStructureOrStructurePointerType(returnType)) {
				std::string typeName;
				ID_T typeID;
				if (returnType->isPointerType()) {
					typeName = GetFullStructureName(returnType->getPointeeType()->getAsCXXRecordDecl());
					typeID = returnType->getPointeeType()->getAsCXXRecordDecl()->getID();
				}
				else {
					typeName = GetFullStructureName(returnType->getAsCXXRecordDecl());
					typeID = returnType->getAsCXXRecordDecl()->getID();
				}
				assert(typeID);
				Structure* typeStructure = structuresTable.Get(typeID);
				if (!typeStructure)
					typeStructure = structuresTable.Insert(typeID, typeName);
				method.SetReturnType(typeStructure);
			}
		}

		currentMethod = parentStructure->InsertMethod(methodID, method);
		sm = result.SourceManager;
		
		// Body - MemberExpr
		auto* body = d->getBody();
		FindMemberExprVisitor visitor; 
		visitor.TraverseStmt(body);
		currentMethod = nullptr;
	}
}

// Handle all the MemberExpr in a method 
bool MethodDeclsCallback::FindMemberExprVisitor::VisitMemberExpr(MemberExpr* memberExpr) {
	auto* decl = memberExpr->getMemberDecl();
	auto type = decl->getType();
	auto* base = memberExpr->getBase();
	bool isMethod = false;

	if (base->getStmtClass() == memberExpr->CXXThisExprClass) { // ignore class' fields
		return true;
	}
		
	auto range = memberExpr->getSourceRange();
	auto srcLocationBegin = MethodDeclsCallback::sm->getPresumedLoc(range.getBegin());
	auto srcLocationEnd = MethodDeclsCallback::sm->getPresumedLoc(range.getEnd());
	SourceInfo locBegin(srcLocationBegin.getFilename(), srcLocationBegin.getLine(), srcLocationBegin.getColumn());
	SourceInfo locEnd(srcLocationEnd.getFilename(), srcLocationEnd.getLine(), srcLocationEnd.getColumn());
	std::string exprString;
	if (decl->getKind() == decl->CXXMethod) {
		auto end = sm->getCharacterData(range.getEnd());
		int openCount = 0, closeCount = 0;
		while (!(openCount == closeCount && openCount)) {
			if (*end == '(') {
				openCount++;
			}
			else if (*end == ')') {
				closeCount++;
			}
			end++;
		}
		exprString = std::string(sm->getCharacterData(range.getBegin()), end);
	}
	else {
		exprString = std::string(sm->getCharacterData(range.getBegin()), sm->getCharacterData(range.getEnd())) + decl->getNameAsString();
	}
	Method::MemberExpr methodMemberExpr(exprString, locEnd, locBegin.GetFileName(), locBegin.GetLine(), locBegin.GetColumn());

	if (!isStructureOrStructurePointerType(type)) {
		if (decl->getKind() == decl->CXXMethod) {
			CXXMethodDecl* methodDecl = (CXXMethodDecl*)decl;
			type = methodDecl->getReturnType();
			isMethod = true;
			if (!isStructureOrStructurePointerType(type)) {
				MethodDeclsCallback::currentMethod->UpdateMemberExpr(methodMemberExpr, locBegin.toString());	// to get the full expr if I have fields with not a class type
				return true;
			}
		}
		else {
			MethodDeclsCallback::currentMethod->UpdateMemberExpr(methodMemberExpr, locBegin.toString());
			return true;
		}
	}

	std::string typeName;
	ID_T typeID; 
	if (type->isPointerType()) {
		typeName = GetFullStructureName(type->getPointeeType()->getAsCXXRecordDecl());
		typeID = type->getPointeeType()->getAsCXXRecordDecl()->getID();
	}
	else {
		typeName = GetFullStructureName(type->getAsCXXRecordDecl());
		typeID = type->getAsCXXRecordDecl()->getID();
	}
	assert(typeID); 
	Structure* typeStructure = structuresTable.Get(typeID);
	if (!typeStructure)
		typeStructure = structuresTable.Insert(typeID, typeName);
	
	Method::Member member(decl->getNameAsString(), typeStructure, locEnd);
	MethodDeclsCallback::currentMethod->InsertMemberExpr(methodMemberExpr, member, locBegin.toString());

	//std::cout << "Member Expr:  " << exprString << "\n";
	//llvm::outs() << "Kind: " << decl->getDeclKindName() << "\n";
	//llvm::outs() << "Name: " << decl->getNameAsString() << "\n";
	//std::cout << "~~~~~~~~~~~~~~~\n\n";
	return true;
}

// Handle Method's Vars and Args
void MethodVarsCallback::run(const MatchFinder::MatchResult& result) {
	if (const VarDecl* d = result.Nodes.getNodeAs<VarDecl>(METHOD_VAR_OR_ARG)) {
		auto* parentMethodDecl = d->getParentFunctionOrMethod();

		// exei mpei gia na to thema pou den vazei ta definitions stis full specialization methods mesa se template
		/*if (d->isFunctionOrMethodVarDecl() && parentMethodDecl->getDeclKind() == d->CXXMethod) {
			auto* parentClass = (CXXRecordDecl*)parentMethodDecl->getParent();
			auto parentClassName = GetFullStructureName(parentClass);
			auto parentMethodName = GetFullMethodName((CXXMethodDecl*)parentMethodDecl);
			Structure* parentStructure = structuresTable.Get(parentClassName);
			Method* parentMethod = parentStructure->GetMethod(parentMethodName);
			std::cout << "Parent: " << parentMethodName << "   " << ((CXXMethodDecl*)parentMethodDecl)->getTemplatedKind() << " !!!!!!!!!  \n";
		}*/


		if (d->isLocalVarDeclOrParm() && parentMethodDecl && parentMethodDecl->getDeclKind() == d->CXXMethod) {	// including params
		//if(d->isFunctionOrMethodVarDecl() && parentMethod->getDeclKind() == d->CXXMethod){		// excluding params	- d->isFunctionOrMethodVarDecl()-> like isLocalVarDecl() but excludes variables declared in blocks?.		
			if (!isStructureOrStructurePointerType(d->getType()))
				return;

			auto* parentClass = (CXXRecordDecl*)parentMethodDecl->getParent();
			auto parentClassName = GetFullStructureName(parentClass);
			auto parentMethodName = GetFullMethodName((CXXMethodDecl*)parentMethodDecl);
			auto parentClassID = parentClass->getID(); 
			auto parentMethodID = ((CXXMethodDecl*)parentMethodDecl)->getID();
			assert(parentClassID);
			assert(parentMethodID);
			Structure* parentStructure = structuresTable.Get(parentClassID);
			Method* parentMethod = parentStructure->GetMethod(parentMethodID);
			if (!parentMethod){
				assert(parentStructure->IsTemplateInstatiationSpecialization());		// insertion speciallization inherite its dependencies from the parent template
				return;
			}
			
			// remove from TemplateInstatiationSpecialization methods the decletarions and arguments 
			if (parentMethod->isTemplateInstatiationSpecialization()) {
				return;
			}

			std::string typeName;
			ID_T typeID; 
			if (d->getType()->isPointerType()) {
				typeName = GetFullStructureName(d->getType()->getPointeeType()->getAsCXXRecordDecl());
				typeID = d->getType()->getPointeeType()->getAsCXXRecordDecl()->getID();
			}
			else {
				typeName = GetFullStructureName(d->getType()->getAsCXXRecordDecl());
				typeID = d->getType()->getAsCXXRecordDecl()->getID();
			}
			assert(typeID);
			Structure* typeStructure = structuresTable.Get(typeID);
			if (!typeStructure)
				typeStructure = structuresTable.Insert(typeID, typeName);
			auto defID = d->getID();
			assert(defID);
			Definition def(defID, d->getQualifiedNameAsString(), typeStructure);
			auto srcLocation = result.SourceManager->getPresumedLoc(d->getLocation());
			def.SetTotalLocation(srcLocation.getFilename(), srcLocation.getLine(), srcLocation.getColumn());
			if (d->isLocalVarDecl()) {
				// remove from templateInstatiation Methods the declarations 
				/*if (((CXXMethodDecl*)parentMethodDecl)->getTemplatedKind() && ((CXXMethodDecl*)parentMethodDecl)->isTemplateInstantiation()) {
					return;
				}*/
				std::cout << "Parent: " << parentMethodName << "  ";
				std::cout << "DEFINITION\n";
				parentMethod->InsertDefinition(defID, def);
			}
			else {
				std::cout << "Parent: " << parentMethodName << "  ";
				std::cout << "ARGUMENT\n";
				parentMethod->InsertArg(defID, def);
			}
		}
	}
}
/*
	Clang Tool Creation
*/
static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nA help message for this specific tool can be added afterwards..\n");

int DependenciesMining::CreateClangTool(int argc, const char** argv, std::vector<std::string> srcs) {
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
	ClangTool Tool(OptionsParser.getCompilations(), srcs);
	
	ClassDeclsCallback classCallback;
	FeildDeclsCallback fieldCallback;
	MethodDeclsCallback methodCallback;
	MethodVarsCallback methodVarCallback;
	MatchFinder Finder;
	Finder.addMatcher(ClassDeclMatcher, &classCallback);
	Finder.addMatcher(FieldDeclMatcher, &fieldCallback); 
	Finder.addMatcher(MethodDeclMatcher, &methodCallback);
	Finder.addMatcher(MethodVarMatcher, &methodVarCallback);
	int result = Tool.run(newFrontendActionFactory(&Finder).get());
	return result;
}

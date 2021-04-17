#pragma warning(disable : 4996)
#pragma warning(disable : 4146)
#include <iostream>
#include <fstream>
#include "SourceLoader.h"
#include "DependenciesMining.h"
#include "GraphGeneration.h"
#include "GraphToJson.h"

static void PrintMainArgInfo(void) {
	std::cout << "Usage: Architecture_Miner [options] <files...>\n\n";
	std::cout << "Options:\n";
	std::cout << "   --src [directory]:    To analyse a whole directory with sources.\n"; 
	std::cout << "                         It can only be used if --cmp-db is not used. \n";
	std::cout << "   --cmp-db [file]:      To analyse a system using a compilation database.\n";
	std::cout << "                         The file must be named \"compile_commands.json\".\n";
	std::cout << "                         It can only be used if --src is not used. \n\n";
	std::cout << "Files (optional):\n";
	std::cout << "   ignored files:        The first parameter is a text file that defines some\n";
	std::cout << "                         file paths in order to be exluded from the analysis.\n";
	std::cout << "   ignored namespaces:   The second parameter is a text file that defines some\n";
	std::cout << "                         namespaces in order to be exluded from the analysis.\n";
	std::cout << "   output file:          The last parameter is a JSON file where the generated\n";
	std::cout << "                         graph will be stored.\n";
}

int main(int argc, const char** argv) {
	if (argc < 2) {
		PrintMainArgInfo();
		return 1;
	}
	std::string option1 = "--src";
	std::string option2 = "--cmp-db";
	const char* cmpDBPath = nullptr;
	std::vector<std::string> srcs;
	if (option1 == argv[1]) { // --src
		sourceLoader::SourceLoader srcLoader(argv[2]);
		srcLoader.LoadSources();
		srcs = srcLoader.GetSources();
	}
	else if (option2 == argv[1]) { //--cmp-db
		cmpDBPath = argv[2];
	}
	else {
		PrintMainArgInfo();
		return 1;
	}
	const char* option = argv[1]; // option = "--src", option = "--cmp-db"
	
	const char* ignoredFilePaths = (argc >= 4) ? argv[3] : "";
	const char* ignoredNamespaces = (argc >= 5) ? argv[4] : "";
	
	std::string fullPath = std::string(__FILE__);
	std::size_t found = fullPath.find_last_of("/\\");
	std::string jsonPath = (argc >= 6) ? argv[5] : fullPath.substr(0, found + 1) + "../../GraphVisualizer/Graph/graph.json";
					
	std::cout << "\nBeginning Analysis\n\n";
	
	std::cout << "Dependency Extraction... ";
	int result = dependenciesMining::CreateClangTool(cmpDBPath, srcs, ignoredFilePaths, ignoredNamespaces);
	std::cout << "Completed\n";

	std::cout << "Dependency Graph Generation... ";
	graph::Graph graph = graphGeneration::GenetareDependenciesGraph(dependenciesMining::structuresTable);
	std::cout << "Completed\n";

	std::cout << "JSON Generation... ";
	std::string json = graphToJson::GetJson(graph);
	std::ofstream jsonFile;
	jsonFile.open(jsonPath);
	jsonFile << json;
 	jsonFile.close();
	std::cout << "Completed\n";
	
	std::cout << "\nAnalysis Completed\n\n";
} 
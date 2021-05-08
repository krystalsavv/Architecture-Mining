# Architecture Miner

This project aims to provide an architecture mining tool that reconstructs a system software architecture from C++ source code based on program class relationships. 
The architecture miner consists of two tools: the dependency graph generator, which extracts the dependencies between classes from source code, represents them as a graph and stores them in JSON format, and the graph visualizer, which takes a dependency graph, analyzes it, and renders it, to represent the software architecture of the given system.

This tool was developed for my MCs thesis at Computer Science Department of University of Crete. The thesis is available here: *It will be available very soon.*

## **Dependency Graph Generator**
### **Dependencies**
* [Clang](https://github.com/llvm/llvm-project)  
* [JsonCpp](https://github.com/open-source-parsers/jsoncpp)

This tool statically analyzes C++ source code using the Clang compiler frontend to extract information about the relationships between program classes, generates a dependency graph that represent classes as nodes and their relationships as edges, and saves it in JSON format.

We represent dependencies that arise as a result of: 
* Inheritance
* Friend methods or classes 
* Nested classes
* Class fields
* Class template parent
* Class template arguments
* Method return type
* Method arguments
* Method locals 
* Member access expression in methods
* Method template arguments

The graph generator is developed in C++ and the source code can be found in the *Architecture-Mining/GraphGenerator* directory.


## **Dependency Graph Visualizer**
### **Dependencies**
* [GoJS](https://gojs.net/latest/index.html)
* [Vue.js](https://vuejs.org/)

The graph visualizer renders the dependency graph and allows the user to configure it.

### **Configurations**
1. Grouping by: 
    * The namespace that classes are defined
    * The source file that classes are defined
    * The relationships between classes using **clustering algorithms** (Louvain, Infomap, Layered Label Propagation)
2. Filtering edges
3. Modify the weight of a dependency type that affects node clustering

The graph visualizer is developed in JavaScript and the source code can be found in the *Architecture-Mining/GraphVisualizer* directory.

## **Demo**
*It will be available very soon.*

## **Using Architecture Miner**
Read the *documentation.pdf* file for a detailed description of the project setup in Windows.

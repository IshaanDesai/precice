// To compile use:
// mpic++ -I$PRECICE_ROOT/src main.cpp -lprecice -o solverdummy

#include <iostream>
#include <sstream>
#include "precice/SolverInterface.hpp"
#include "precice/Constants.hpp"

int main (int argc, char **argv)
{
  std::cout << "Starting SolverDummy...\n";
  int commRank = 0;
  int commSize = 1;

  using namespace precice;
  using namespace precice::constants;

  if (argc != 6){
    std::cout << "Usage: ./solverdummy configFile solverName meshName\n";
    std::cout << '\n';
    std::cout << "Parameter description\n";
    std::cout << "  configurationFile: Path and filename of preCICE configuration\n";
    std::cout << "  solverName:        SolverDummy participant name in preCICE configuration\n";
    std::cout << "  meshName:          Mesh in preCICE configuration that carries read and write data\n";
    std::cout << "  writeDataName:          Name of write data\n";
    std::cout << "  readDataName:          Name of read data\n";
    return 1;
}

  std::string configFileName(argv[1]);
  std::string solverName(argv[2]);
  std::string meshName(argv[3]);
  std::string writeDataName(argv[4]);
  std::string readDataName(argv[5]);
  int N = 1;

  SolverInterface interface(solverName, commRank, commSize);
  interface.configure(configFileName);

  int meshID = interface.getMeshID(meshName);
  int dimensions = interface.getDimensions();
  std::vector<std::vector<double>> vertices(N, std::vector<double>(dimensions, 0)); 
  std::vector<int> valueIndices(N,0);
  int writeDataID = interface.getDataID(writeDataName, meshID);
  int readDataID = interface.getDataID(readDataName, meshID);

  for(int i=0; i<N; i++){
    valueIndices[i] = interface.setMeshVertex(meshID, vertices[i].data());
  }

  double dt = interface.initialize();

  double readValue = -1;
  double writeValue = 5;

  while (interface.isCouplingOngoing()){

    if (interface.isActionRequired(actionWriteIterationCheckpoint())){
      std::cout << "DUMMY: Writing iteration checkpoint\n";
      interface.fulfilledAction(actionWriteIterationCheckpoint());
    }

    std::cout << "before\n";
    std::cout << "readValue:" << readValue << "\n";
    std::cout << "writeValue:" << writeValue << "\n";

    interface.writeScalarData(writeDataID, valueIndices[0], writeValue);
    dt = interface.advance(dt);
    interface.readScalarData(readDataID, valueIndices[0], readValue);

    std::cout << "after\n";
    std::cout << "readValue:" << readValue << "\n";
    std::cout << "writeValue:" << writeValue << "\n";

    if (interface.isActionRequired(actionReadIterationCheckpoint())){
      std::cout << "DUMMY: Writing iteration checkpoint\n";
      interface.fulfilledAction(actionReadIterationCheckpoint());
    }
    else{
      std::cout << "DUMMY: Advancing in time\n";
    }
  }

  interface.finalize();
  std::cout << "DUMMY: Closing C++ solver dummy...\n";

  return 0;
}

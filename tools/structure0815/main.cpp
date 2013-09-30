#include <iostream>
#include <sstream>
#include "Structure0815.hpp"
#include "Tests.hpp"
#include "Globals.hpp"
#include "precice/SolverInterface.hpp"
#include "utils/Dimensions.hpp"
#include "tarch/la/WrappedVector.h"
#include "utils/Globals.hpp"

using precice::utils::DynVector;
using namespace precice;
using tarch::la::raw;

/**
 * @brief Runs structure0815
 */
int main(int argc, char **argv)
{
  STRUCTURE_INFO("Starting Structure0815");

  if (argc != 2){
    STRUCTURE_INFO("Usage: ./structure0815 configurationFileName/tests");
    abort();
  }
  std::string configFileName(argv[1]);

  if (configFileName == std::string("tests")){
    Tests tests;
    tests.run();
    return 1;
  }

  std::string meshName("WetSurface");

  SolverInterface cplInterface("Structure0815", 0, 1);
  cplInterface.configure(configFileName);
  double dt = cplInterface.initialize();

  std::string writeCheckpoint(constants::actionWriteIterationCheckpoint());
  std::string readCheckpoint(constants::actionReadIterationCheckpoint());

  int dimensions = cplInterface.getDimensions();
  double density = 500.0;
  DynVector gravity (dimensions, 0.0); //-9.81;
  gravity(1) = -9.81;

  STRUCTURE_INFO("Density = " << density);
  STRUCTURE_INFO("Gravity = " << gravity);

  //DynVector translVelocityChange(dimensions, 0.0);

  if ( not cplInterface.hasMesh(meshName) ){
    STRUCTURE_INFO("Mesh \"" << meshName << "\" required for coupling!");
    exit(-1);
  }
  MeshHandle handle = cplInterface.getMeshHandle(meshName);

  int velocitiesID = -1;
  int forcesID = -1;
  int displacementsID = -1;
  int displDeltasID = -1;
  int velocityDeltasID = -1;

  if (not cplInterface.hasData(constants::dataForces())){
    STRUCTURE_INFO("Data \"Forces\" required for coupling!");
    exit(-1);
  }
  forcesID = cplInterface.getDataID(constants::dataForces());
  if (cplInterface.hasData(constants::dataVelocities())){
    velocitiesID = cplInterface.getDataID(constants::dataVelocities());
  }
  if (cplInterface.hasData(constants::dataDisplacements())){
    displacementsID = cplInterface.getDataID(constants::dataDisplacements());
  }
  if (cplInterface.hasData("DisplacementDeltas")){
    displDeltasID = cplInterface.getDataID("DisplacementDeltas");
  }
  if (cplInterface.hasData("VelocityDeltas")){
    velocityDeltasID = cplInterface.getDataID("VelocityDeltas");
  }

  DynVector fixture(dimensions, 0.0);
  bool fixStructure = false;
  STRUCTURE_INFO("Fix structure = " << fixStructure);

  DynVector nodes(handle.vertices().size()*dimensions);
  tarch::la::DynamicVector<int> faces;
  size_t iVertex = 0;
  VertexHandle vertices = handle.vertices();
  foriter (VertexIterator, it, vertices){
    for (int i=0; i < dimensions; i++){
      nodes[iVertex*dimensions+i] = it.vertexCoords()[i];
    }
    iVertex++;
  }
  if (dimensions == 2){
    faces.append(handle.edges().size()*2, 0);
    EdgeHandle edges = handle.edges();
    int iEdge = 0;
    foriter (EdgeIterator, it, edges){
      for (int i=0; i < 2; i++){
        faces[iEdge*2+i] = it.vertexID(i);
      }
      iEdge++;
    }
  }
  else {
    assertion1(dimensions == 3, dimensions);
    faces.append(handle.triangles().size()*3, 0);
    TriangleHandle triangles = handle.triangles();
    int iTriangle = 0;
    foriter (TriangleIterator, it, triangles){
      for (int i=0; i < 3; i++){
        faces[iTriangle*3+i] = it.vertexID(i);
      }
      iTriangle++;
    }
  }

  tarch::la::DynamicVector<int> vertexIDs(handle.vertices().size());
  for (int i=0; i < handle.vertices().size(); i++){
    vertexIDs[i] = i;
  }

  Structure0815 structure(dimensions, density, gravity, nodes, faces);

  if (fixStructure){ // Fix the structure (only rotations possible)
    structure.fixPoint(fixture);
  }

  // Main timestepping/iteration loop
  while (cplInterface.isCouplingOngoing()){
    if (cplInterface.isActionRequired(writeCheckpoint)){
      cplInterface.fulfilledAction(writeCheckpoint);
    }

    cplInterface.readBlockVectorData(forcesID, vertexIDs.size(), raw(vertexIDs), raw(structure.forces()));

    structure.iterate(dt);

    if (velocitiesID != -1){
      cplInterface.writeBlockVectorData(velocitiesID, vertexIDs.size(), raw(vertexIDs), raw(structure.velocities()));
    }
    if (displacementsID != -1){
      cplInterface.writeBlockVectorData(displacementsID, vertexIDs.size(), raw(vertexIDs), raw(structure.displacements()));
    }
    if (displDeltasID != -1){
      cplInterface.writeBlockVectorData(displDeltasID, vertexIDs.size(), raw(vertexIDs), raw(structure.displacementDeltas()));
    }
    if (velocityDeltasID != -1){
      cplInterface.writeBlockVectorData(velocityDeltasID, vertexIDs.size(), raw(vertexIDs), raw(structure.velocityDeltas()));
    }

    dt = cplInterface.advance(dt);

    if (cplInterface.isActionRequired(readCheckpoint)){
      STRUCTURE_DEBUG("Loading checkpoint");
      cplInterface.fulfilledAction(readCheckpoint);
    }
    else {
      structure.timestep(dt);
    }
  }

  STRUCTURE_DEBUG("Finalizing coupling");
  cplInterface.finalize();

  STRUCTURE_INFO("Exiting Structure0815");

  return 1;
}

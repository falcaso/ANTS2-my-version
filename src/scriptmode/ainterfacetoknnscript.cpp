#include "ainterfacetoknnscript.h"
#include "nnmoduleclass.h"

#include <QDebug>
#include <limits>

AInterfaceToKnnScript::AInterfaceToKnnScript(NNmoduleClass* knnModule) : knnModule(knnModule) {}

QVariant AInterfaceToKnnScript::getNeighbours(int ievent, int numNeighbours)
{    
    QVariant res = knnModule->ScriptInterfacer->getNeighbours(ievent, numNeighbours);
    if (res == QVariantList())
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
    }
    return res;
}

void AInterfaceToKnnScript::filterByDistance(int numNeighbours, double distanceLimit, bool filterOutEventsWithSmallerDistance)
{
    bool ok = knnModule->ScriptInterfacer->filterByDistance(numNeighbours, distanceLimit, filterOutEventsWithSmallerDistance);
    if (!ok)
    {
        abort("kNN module reports fail:\n" + knnModule->ScriptInterfacer->ErrorString);
        return;
    }
}

void AInterfaceToKnnScript::SetSignalNormalizationType(int type_0None_1sum_2quadraSum)
{
  knnModule->ScriptInterfacer->SetSignalNormalization(type_0None_1sum_2quadraSum);
}

void AInterfaceToKnnScript::clearCalibrationEvents()
{
  knnModule->ScriptInterfacer->clearCalibration();
}

bool AInterfaceToKnnScript::setGoodScanEventsAsCalibration()
{
  return knnModule->ScriptInterfacer->setCalibration(true);
}

bool AInterfaceToKnnScript::setGoodReconstructedEventsAsCalibration()
{
  return knnModule->ScriptInterfacer->setCalibration(false);
}

int AInterfaceToKnnScript::countCalibrationEvents()
{
  return knnModule->ScriptInterfacer->countCalibrationEvents();
}

double AInterfaceToKnnScript::getCalibrationEventX(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventY(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventZ(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventX(ievent);
}

double AInterfaceToKnnScript::getCalibrationEventEnergy(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
}

QVariant AInterfaceToKnnScript::getCalibrationEventXYZE(int ievent)
{
  QVariantList l;
  l << knnModule->ScriptInterfacer->getCalibrationEventX(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventY(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventZ(ievent) <<
       knnModule->ScriptInterfacer->getCalibrationEventE(ievent);
  return l;
}

QVariant AInterfaceToKnnScript::getCalibrationEventSignals(int ievent)
{
  return knnModule->ScriptInterfacer->getCalibrationEventSignals(ievent);
}

#include "ainterfacetoannscript.h"

#include <QJsonArray>
//#include <QJsonObject>
#include <QDebug>

/*===========================================================================*/
AInterfaceToANNScript::AInterfaceToANNScript()
{
  resetConfigToDefault();
}

/*===========================================================================*/
AInterfaceToANNScript::~AInterfaceToANNScript()
{
  clearTrainingData();
}

/*===========================================================================*/
void AInterfaceToANNScript::clearTrainingData()
{
  Input.clear(); Input.squeeze();
  Output.clear(); Output.squeeze();

}

/*===========================================================================*/
//! Reset the training data.
void AInterfaceToANNScript::newNetwork()
{
  clearTrainingData();
  qDebug() << "New NN created.";
}

/*===========================================================================*/
QString AInterfaceToANNScript::configure(QVariant configObject)
{
  qDebug() << configObject.typeName();
  if ( QString(configObject.typeName()) != "QVariantMap")  //critical error!
  {
      abort("ANN script: configure function requirs an object-type argument");
      return "";
  }

  QVariantMap map = configObject.toMap();
  Config = QJsonObject::fromVariantMap(map);
  qDebug() << "User config:" << Config;

  if (!Config.contains("type")){
   abort("FANN Type - ('type' key) has to be defined in ANN config");
   return "";
  }

  QString norm = Config["norm"].toString();
  qDebug() << "Norm:"<<norm;
  int val = Config["val"].toInt();
  double val2 = Config["afloat"].toDouble();

  qDebug() << val;
  qDebug() << val2;

  //.....

  return "Anything useful you want to communicate back: e.g. warnings etc";
}

/*===========================================================================*/
void AInterfaceToANNScript::resetConfigToDefault()
{
  Config = QJsonObject();
  Config["type"]="CASCADE";
  Config["trainAlgorithm"]="RETRO PROPAGATION";
  Config["outputActivationFunc"]="SIGMOID";
  Config["errorFunction"]="LINEAR";
  Config["stopFunction"]="BIT";
  //...........................................................................
  Config["bitFailLimit"]=0.1; // no default.
  Config["cascade_outputChangeFraction"]=0.01; // default
  Config["cascade_candidateChangeFraction"]=0.01; // default
  Config["cascade_outputStagnationEpochs"]=12; // default
  Config["cascade_minOutEpochs"]=50; // default
  Config["cascade_maxOutEpochs"]=150; // default
  Config["cascade_candidateStagnationEpochs"]=12; // default
  Config["cascade_minCandEpochs"]=50; // default
  Config["cascade_maxCandEpochs"]=150; // default
  Config["cascade_candidateLimit"]=1000; // default
  Config["cascade_candidateGroups"]=2; // default
  Config["cascade_weightMultiplier"]=0.4; // default
  Config["cascade_setActivationSteepness"]=QJsonArray({ 0.25, 0.5, 0.75, 1. });
  Config["cascade_setActivationFunctions"]=QJsonArray({"SIGMOID","SIGMOID SYMMETRIC",
   "GAUSSIAN","GAUSSIAN SYMMETRIC","ELLIOT","ELLIOT SYMMETRIC"});
}

/*===========================================================================*/
QVariant AInterfaceToANNScript::getConfig()
{
  const QVariantMap res = Config.toVariantMap();
  return res;
}

/*===========================================================================*/
void AInterfaceToANNScript::addTrainingInput(QVariant arrayOfArrays)
{
  //unpacking data
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Input, fixedSize) ) return;
  qDebug() << "New training input is set to:" << Input;

}

/*===========================================================================*/
void AInterfaceToANNScript::addTrainingOutput(QVariant arrayOfArrays)
{
  //unpacking data
  int fixedSize = Output.isEmpty() ? -1 : Output.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Output, fixedSize) ) return;
  qDebug() << "New training output is set to:" << Output;
}

/*===========================================================================*/
QString AInterfaceToANNScript::train()
{
  if (Input.isEmpty() || Output.isEmpty()) return "Training data empty";
  if (Input.size() != Output.size()) return "Training data size mismatch";

  if (Config["type"].toString()=="CASCADE"){
      afann.createCascade(Input.first().size(),Output.first().size());

      afann.cascade_outputChangeFraction(Config["cascade_outputChangeFraction"].toDouble());
      afann.cascade_candidateChangeFraction(Config["cascade_candidateChangeFraction"].toDouble());
      afann.cascade_outputStagnationEpochs(Config["cascade_outputStagnationEpochs"].toInt());
      afann.cascade_minOutEpochs(Config["cascade_minOutEpochs"].toInt());
      afann.cascade_maxOutEpochs(Config["cascade_maxOutEpochs"].toInt());
      afann.cascade_candidateStagnationEpochs(Config["cascade_candidateStagnationEpochs"].toInt());
      afann.cascade_minCandEpochs(Config["cascade_minCandEpochs"].toInt());
      afann.cascade_maxCandEpochs(Config["cascade_maxCandEpochs"].toInt());
      afann.cascade_candidateLimit(Config["cascade_candidateLimit"].toInt());
      afann.cascade_candidateGroups(Config["cascade_candidateGroups"].toInt());
      afann.cascade_weightMultiplier(Config["cascade_weightMultiplier"].toDouble());

      //  Config["cascade_setActivationSteepness"] = QJsonArray({ 0.25, 0.5, 0.75, 1. });
      //  Config["cascade_setActivationFunctions"]=QJsonArray({"FANN_SIGMOID",
      //   "FANN_SIGMOID_SYMMETRIC","FANN_GAUSSIAN","FANN_GAUSSIAN_SYMMETRIC",
      //   "FANN_ELLIOT","FANN_ELLIOT_SYMMETRIC"});

  }

  afann.trainAlgorithm(NeuralNetworksModule::trainAlgorithms
   .option(Config["trainAlgorithm"].toString().toStdString()));
  afann.outputActivationFunc(NeuralNetworksModule::activationFunctions
   .option(Config["outputActivationFunc"].toString().toStdString()));
  afann.errorFunction(NeuralNetworksModule::trainErrorFunctions
   .option(Config["errorFunction"].toString().toStdString()));
  afann.stopFunction(NeuralNetworksModule::trainStopFunctions
   .option(Config["stopFunction"].toString().toStdString()));

  afann.bitFailLimit(Config["bitFailLimit"].toDouble());

  return ""; //success
}

/*===========================================================================*/
QVariant AInterfaceToANNScript::process(QVariant arrayOfArrays)
{
  //unpacking data
  QVector< QVector<float> > Data;
  int fixedSize = Input.isEmpty() ? -1 : Input.first().size();
  if ( !convertQVariantToVectorOfVectors(&arrayOfArrays, &Data, fixedSize) ) return QVariant();
  qDebug() << "received to process:" << Data;

  //.....

  //packing the results to send back to the script
  QVector< QVector<float> > dummy; dummy << QVector<float>(5,0) << QVector<float>(5,1) << QVector<float>(5, 2);
  QVariantList l;
  for (int i=0; i<dummy.size(); i++)
    {
      const QVector<float>& thisOne = dummy.at(i);
      QVariantList ll;
      for (int ii=0; ii<thisOne.size(); ii++) ll << thisOne.at(ii);
      l << QVariant(ll);
    }
  return l;
}

/*===========================================================================*/
bool AInterfaceToANNScript::convertQVariantToVectorOfVectors(QVariant *var, QVector<QVector<float> > *vec, int fixedSize)
{
  if ( QString(var->typeName()) != "QVariantList")
  {
      abort("ANN script: array (or array of arrays) is required as an argument");
      return false;
  }
  QVariantList tmp = var->toList();
  QJsonArray arr = QJsonArray::fromVariantList(tmp);
  for (int i=0; i<arr.size(); i++)
    {
      QJsonArray el = arr[i].toArray();
      if (  el.isEmpty() || (fixedSize!=-1 && el.size()!=fixedSize)  )
        {
          abort("ANN script: Invalid argument or size mismatch");
          return false;
        }
      QVector<float> tmp;
      for (int ii=0; ii<el.size(); ii++) tmp << el[ii].toDouble();
      vec->append(tmp);
    }
  return true;
}

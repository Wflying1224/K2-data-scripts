#include <matchSeries.h>
#include <cellCenteredGrid.h>
#include <multiStreambuf.h>
#include <paramReg.h>
#include <mutualInformation.h>

/**
 * \author Berkels
 */
template <typename _ConfiguratorType, template<class> class RegistrationConfiguratorType, template<class> class RegularizationConfiguratorType = qc::DirichletRegularizationConfigurator >
class NonDyadicRegistrationMultilevelDescent : public qc::RegistrationInterface<_ConfiguratorType> {
public:
  typedef _ConfiguratorType ConfiguratorType;
  typedef typename ConfiguratorType::RealType RealType;
  typedef typename ConfiguratorType::ArrayType ArrayType;
  typedef qc::QuocConfiguratorTraitMultiLin<RealType, ConfiguratorType::Dim, aol::GaussQuadrature<RealType,ConfiguratorType::Dim,3> > DyadicConfiguratorType;
  typedef qc::StandardRegistrationMultilevelDescent<DyadicConfiguratorType, RegistrationConfiguratorType<DyadicConfiguratorType>, RegularizationConfiguratorType<DyadicConfiguratorType> > DyadicRegistrationMultilevelDescentType;
  const aol::ParameterParser &_parser;
  aol::ParameterParser _mldParser;
  aol::DeleteFlagPointer<DyadicRegistrationMultilevelDescentType> _pMld;
  qc::MultiArray<RealType, ConfiguratorType::Dim> _phi;
public:
  NonDyadicRegistrationMultilevelDescent ( const aol::ParameterParser &Parser )
    : qc::RegistrationInterface<ConfiguratorType> ( Parser.getString ( "reference" ).c_str(), Parser.getString ( "template" ).c_str(), Parser.getDouble ( "lambda" ), Parser.checkAndGetBool ( "dontNormalizeInputImages" ) ),
      _parser ( Parser ),
      _mldParser ( Parser ),
      _phi ( this->_grid ) {
    const int depth = qc::logBaseTwo ( this->_grid.getNumX() - 1 );
    _mldParser.changeVariableValue ( "stopLevel", depth );
    _mldParser.changeVariableValue ( "precisionLevel", depth );
    _pMld.reset ( new DyadicRegistrationMultilevelDescentType ( _mldParser ), true );
  }

  void loadAndPrepareImage ( const char* Filename, ArrayType &Dest, const bool NoScaling ) const {
    Dest.load ( Filename );
    if ( NoScaling == false )
      Dest.scaleValuesTo01();
  }

  void loadRefOrTemplate ( const char* Filename, const qc::REGISTRATION_INPUT_TYPE Input, const bool NoScaling ) {
    _pMld->loadRefOrTemplate( Filename, Input, NoScaling );
    loadAndPrepareImage ( Filename, ( Input == qc::REFERENCE ) ? this->getNonConstRefImageReference() : this->getNonConstTemplImageReference(), NoScaling );
  }

  void setTemplate ( const ArrayType &Template ) {
    this->getNonConstTemplImageReference() = Template;
    _pMld->getTemplImageMLAReference()[_pMld->getMaxGridDepth()].resampleFrom ( Template );
    _pMld->getTemplImageMLAReference().levRestrict ( 0, _pMld->getMaxGridDepth() );
  }

  void setReference ( const ArrayType &Reference ) {
    this->getNonConstRefImageReference() = Reference;
    _pMld->getRefImageMLAReference()[_pMld->getMaxGridDepth()].resampleFrom ( Reference );
    _pMld->getRefImageMLAReference().levRestrict ( 0, _pMld->getMaxGridDepth() );
  }

  void setTransformation ( const qc::MultiArray<RealType, ConfiguratorType::Dim> &Transformation ) {
    _phi = Transformation;
  }

  void getTransformation ( qc::MultiArray<RealType, ConfiguratorType::Dim> &Transformation ) const {
    Transformation = _phi;
  }

  void addTransformationTo ( qc::MultiArray<RealType, ConfiguratorType::Dim> &Transformation ) const {
    Transformation += _phi;
  }

  void setTransformationToZero ( ) {
    _phi.setZero( );
  }

  const char* getSaveDirectory ( ) const {
    return _pMld->getSaveDirectory();
  }

  void setSaveDirectory ( const char *SaveDirectory ) {
    _pMld->setSaveDirectory( SaveDirectory );
  }

  void makeAndSetSaveDirectory ( const char *SaveDirectory ) {
    _pMld->makeAndSetSaveDirectory ( SaveDirectory );
  }

  int getMaxGridDepth( ) const {
    return _pMld->getMaxGridDepth() + ( ( _pMld->getInitializerRef().getSize() != this->_grid.getSize() ) ? 1 : 0 );
  }

  void solve ( ) {
    cerr << aol::generateCurrentTimeAndDateString() << endl;

    _pMld->solve ( );

    if ( _pMld->getMaxGridDepth() != getMaxGridDepth() ) {
      qc::MultiArray<RealType, ConfiguratorType::Dim> approxPhi ( _pMld->getTransformationReference(), aol::FLAT_COPY );
      _phi.resampleFrom ( approxPhi );

      const RegistrationConfiguratorType<ConfiguratorType> regisConfig ( _parser );
      const RegularizationConfiguratorType<ConfiguratorType> regulConfig ( this->_grid, _parser );
      qc::StandardRegistration<ConfiguratorType, ArrayType, RegistrationConfiguratorType<ConfiguratorType>, RegularizationConfiguratorType<ConfiguratorType>, aol::H1GradientDescent<ConfiguratorType, aol::MultiVector<RealType> > > stdRegistration ( this->getRefImageReference(), this->getTemplImageReference(), regisConfig, regulConfig, this->_lambda );
      stdRegistration.findTransformation ( _phi );

      qc::RegistrationStepSaver<ConfiguratorType>
        stepSaver( this->_grid, this->getRefImageReference(),  this->getTemplImageReference(), _parser.getInt ( "checkboxWidth" ) );
      stepSaver.setSaveName ( aol::strprintf ( "_%02d", _pMld->getMaxGridDepth() + 1 ).c_str() );
      stepSaver.setSaveDirectory ( _pMld->getSaveDirectory() );
      stepSaver.saveStep ( _phi, -1 );
    }
  }

  void solveAndProlongToMaxDepth ( ) {
    solve ( );
  }
};

template <typename ConfiguratorType, typename RegistrationType>
void matchSeries ( aol::ParameterParser &Parser, int argc, char **argv ) {
  typedef typename ConfiguratorType::RealType RealType;
  const int numExtraStages = Parser.getInt ( "numExtraStages" );
  const RealType extraStageslambdaFactor = Parser.getDouble ( "extraStagesLambdaFactor" );

  // This creates the save directory and dumps the paramters to a file in that dir.
  qc::DefaultArraySaver<RealType, ConfiguratorType::Dim> saver;
  saver.initFromParser ( Parser, true );
  aol::AdditionalOutputToFile addOut ( saver.createSaveName ( "", ".txt", -1, "log" ).c_str() );

  if ( Parser.hasVariable ( "templateNamePattern" ) ) {
    if ( Parser.hasVariable ( "reference" ) )
      throw aol::Exception( "Paramater \"reference\" may not be specified if \"templateNamePattern\" is used.", __FILE__, __LINE__);
    else
    {
      // Use the first of the templates as reference image
      Parser.addVariable ( "reference", aol::strprintf ( Parser.getString ( "templateNamePattern" ).c_str(), Parser.getInt ( "templateNumOffset" ) ).c_str() );
      Parser.changeVariableValue ( "templateNumOffset", aol::strprintf ( "%d", Parser.getInt ( "templateNumOffset" ) + Parser.getInt ( "templateNumStep" ) ).c_str() );
      Parser.changeVariableValue ( "numTemplates", aol::strprintf ( "%d", Parser.getInt ( "numTemplates" ) - 1 ).c_str() );
    }
  }

  // qc::RegistrationMultilevelDescentInterfaceBase expects the parser to have a valid "template" parameter.
  // SeriesMatching itself doesn't need it, so just add the current "reference" value as valid dummy value for "template".
  if ( Parser.hasVariable ( "template" ) == false )
    Parser.addVariable ( "template", Parser.getString ( "reference" ).c_str() );

  aol::ParameterParser parserStage1 ( Parser );
  parserStage1.changeVariableValue ( "saveDirectory", ( string ( saver.getSaveDirectory() ) + "stage1/" ).c_str() );

  SeriesMatching<RegistrationType> mldS1 ( parserStage1 );
  const typename SeriesMatching<RegistrationType>::ACTION actionType = ( argc > 2 ) ? static_cast<typename SeriesMatching<RegistrationType>::ACTION> ( atoi ( argv[2] ) ) : SeriesMatching<RegistrationType>::MATCH_AND_AVERAGE_SERIES;
  if ( Parser.checkAndGetBool ( "skipStage1" ) == false )
    mldS1.doAction( actionType );
  const string averageName = string ( Parser.checkAndGetBool ( "useMedianAsNewTarget" ) ? "median" : "average" ) + qc::getDefaultArraySuffix ( ConfiguratorType::Dim );
  string lastAverageFileName = string ( mldS1.getSaveDirectory() ) + averageName;

  if ( numExtraStages > 0 ) {
    aol::ParameterParser parserStage2 ( Parser );
    parserStage2.changeVariableValue ( "templateNumOffset", aol::strprintf ( "%d", Parser.getInt ( "templateNumOffset" ) - Parser.getInt ( "templateNumStep" ) ).c_str() );
    parserStage2.changeVariableValue ( "numTemplates", aol::strprintf ( "%d", Parser.getInt ( "numTemplates" ) + 1 ).c_str() );
    parserStage2.changeVariableValue ( "lambda", aol::strprintf ( "%f", Parser.getDouble ( "lambda" ) * extraStageslambdaFactor ).c_str() );
    if ( Parser.checkAndGetBool ( "cropInput" ) || Parser.checkAndGetBool ( "resizeInput" ) )
      parserStage2.changeVariableValue ( "dontResizeOrCropReference", 1 );

    for ( int i = 0; i < numExtraStages; ++i ) {
      aol::ParameterParser parserStageN ( parserStage2 );
      parserStageN.changeVariableValue ( "saveDirectory", aol::strprintf( "%sstage%d/", saver.getSaveDirectory(), i+2 ).c_str() );
      parserStageN.changeVariableValue ( "reference", lastAverageFileName.c_str() );
      parserStageN.addVariable ( "stage", i+2 );

      SeriesMatching<RegistrationType> mldSN ( parserStageN, Parser.checkAndGetBool ( "reuseStage1Results" ) );
      mldSN.doAction( actionType );
      lastAverageFileName = string ( mldSN.getSaveDirectory() ) + averageName;
    }
  }
}

typedef double RType;
const qc::Dimension DimensionChoice = qc::QC_2D;
//typedef qc::QuocConfiguratorTraitMultiLin<RType, DimensionChoice, aol::GaussQuadrature<RType,DimensionChoice,3> > ConfType;
typedef qc::RectangularGridConfigurator<RType, DimensionChoice, aol::GaussQuadrature<RType,DimensionChoice,3>, qc::CellCenteredCubicGrid<DimensionChoice> > ConfType;

int main( int argc, char **argv ) {
  try {
    aol::StopWatch watch;
    watch.start();

    aol::ParameterParser parser( argc > 1 ? argv[1] : "matchSeries.par" );

    const int deformationModel = parser.getIntOrDefault ( "deformationModel", 0 );
    switch ( deformationModel ) {
      case 0:
        {
          typedef qc::StandardRegistrationMultilevelDescent<ConfType, qc::NCCRegistrationConfigurator<ConfType> > RegisType;
          matchSeries<ConfType, RegisType>( parser, argc, argv );
        }
        break;
      case 1:
        {
          typedef qc::ParametricRigidBodyMotion2D<ConfType> ParametricDefType;
          typedef qc::ParametricRegistrationMultilevelDescent<ConfType, ParametricDefType, qc::ParametricNCCEnergy<ConfType, ParametricDefType> > RegisType;
          matchSeries<ConfType, RegisType>( parser, argc, argv );
        }
        break;
      case 2:
        {
          typedef qc::ParametricTranslation<ConfType> ParametricDefType;
          typedef qc::ParametricRegistrationMultilevelDescent<ConfType, ParametricDefType, qc::ParametricNCCEnergy<ConfType, ParametricDefType> > RegisType;
          matchSeries<ConfType, RegisType>( parser, argc, argv );
        }
        break;
      case 3:
        {
          typedef qc::StandardRegistrationMultilevelDescent<ConfType, qc::MIRegistrationConfigurator<ConfType> > RegisType;
          matchSeries<ConfType, RegisType>( parser, argc, argv );
        }
        break;
      case 4:
        {
          typedef qc::StandardRegistrationMultilevelDescent<ConfType, qc::MIRegistrationConfigurator<ConfType>, qc::DirichletLaplaceConfigurator<ConfType> > RegisType;
          matchSeries<ConfType, RegisType>( parser, argc, argv );
        }
        break;
      case 5:
      {
        typedef qc::ParametricRigidBodyMotion2D<ConfType> ParametricDefType;
        typedef qc::ParametricRegistrationMultilevelDescent<ConfType, ParametricDefType, qc::ParametricMIEnergy<ConfType, ParametricDefType> > RegisType;
        matchSeries<ConfType, RegisType>( parser, argc, argv );
      }
        break;
      default:
        throw aol::Exception ( "Unknown mode", __FILE__, __LINE__ );
    }

    watch.stop();
    watch.printReport ( cerr );

  }
  catch ( aol::Exception &el ) {
    el.dump();
  }
  aol::callSystemPauseIfNecessaryOnPlatform();
  return 0;
}

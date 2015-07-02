#ifndef __MATCHSERIES_H
#define __MATCHSERIES_H

#include <registration.h>
#include <dm3Import.h>

/**
 * \author Berkels
 */
template <typename RegistrationType>
class SeriesMatching {
  typedef typename RegistrationType::ConfiguratorType ConfiguratorType;
  typedef typename RegistrationType::TransformationDOFType TransformationDOFType;
  typedef typename ConfiguratorType::ArrayType ArrayType;
  typedef typename ConfiguratorType::RealType RealType;
  static const qc::Dimension Dim = ConfiguratorType::Dim;

  const aol::ParameterParser &_parser;
  RegistrationType _registrationAlgo;

  const bool _loadStageOneResults;
  const bool _reduceDeformations;
  const bool _useAltStartLevel;
  const int _stage;
public:
  enum ACTION {
    MATCH_AND_AVERAGE_SERIES,
    ONLY_AVERAGE_SERIES,
    ANALYZE_DEFORMATTIONS,
    APPLY_DEFORMATTION
  };

  SeriesMatching ( const aol::ParameterParser &Parser, const bool LoadStageOneResults = false )
    : _parser ( Parser ),
      _registrationAlgo ( Parser ),
      _loadStageOneResults ( LoadStageOneResults ),
      _reduceDeformations ( _parser.checkAndGetBool ( "reduceDeformations" ) ),
      _useAltStartLevel ( _parser.hasVariable ( "altStartLevel" ) && ( _parser.getInt ( "altStartLevel" ) != _parser.getInt ( "startLevel" ) ) ),
      _stage ( _parser.hasVariable ( "stage" ) ? _parser.getInt ( "stage" ) : 1 ) { }

  void createTemplateFileNameList ( std::vector<std::string> &FileNames ) const {
    std::set<int> skipNumsSet;
    if ( _parser.hasVariable ( "templateSkipNums" ) ) {
      aol::Vector<int> skipNums;
      _parser.getIntVec ( "templateSkipNums", skipNums );
      for ( int i = 0; i < skipNums.size(); ++i )
        skipNumsSet.insert ( skipNums[i] );
    }

    aol::Vector<int> templateNums;
    if ( _parser.hasVariable ( "numTemplates" ) ) {
      const int maxNumTemplates = _parser.getInt ( "numTemplates" );
      for ( int i = 0; i < maxNumTemplates; ++i ) {
        const int index = _parser.getInt ( "templateNumStep" ) * i + _parser.getInt ( "templateNumOffset" );
        if ( skipNumsSet.find ( index ) == skipNumsSet.end() )
          templateNums.pushBack ( index );
      }
    }

    const int numTemplateImages = _parser.hasVariable ( "numTemplates" ) ? templateNums.size() : _parser.getDimSize ( "templates", 0 );
    FileNames.resize ( numTemplateImages );
    cerr << "Using templates images ---------------------------------------\n";
    const string templateDirectory = _parser.hasVariable ( "templatesDirectory" ) ? _parser.getString ( "templatesDirectory" ) : "";
    for ( int i = 0; i < numTemplateImages; ++i ) {
      if ( _parser.hasVariable ( "numTemplates" ) ) {
        FileNames[i] =  aol::strprintf ( _parser.getString ( "templateNamePattern" ).c_str(), templateNums[i] );
      }
      else
        FileNames[i] = templateDirectory + _parser.getString ( "templates", i ).c_str();
      cerr << FileNames[i] << endl;
    }
    cerr << "--------------------------------------------------------------\n";
  }

  int getNumTemplates ( ) const {
    // We don't need these filenames here, but this is a convenient way to calculate
    // the number of templates we have.
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    return templateFileNames.size();
  }

  string createDeformationBaseFileName ( const char *InputDirectory, const int DeformationNumber ) const {
    if ( _useAltStartLevel && ( DeformationNumber == 0 ) )
      return aol::strprintf ( "%sdeformation-%03d", InputDirectory, DeformationNumber );
    else {
      const bool refinedDeformation = ( ( _parser.checkAndGetBool ( "dontAccumulateDeformation" ) == false ) && ( DeformationNumber > 0 ) );
      return aol::strprintf ( "%s%d%s/deformation_%02d", InputDirectory, DeformationNumber, refinedDeformation ? "-r" : "", refinedDeformation ? _registrationAlgo.getMaxGridDepth() : _parser.getInt ( "stopLevel" ) );
    }
  }

  string createDeformationFileName ( const char *InputDirectory, const int DeformationNumber ) const {
    return ( createDeformationBaseFileName ( InputDirectory, DeformationNumber ) + _registrationAlgo.getDeformationFileNameSuffix() );
  }

  void matchSeries ( ) {
    // We will temporarily change the save directory, so store the original value.
    const string origSaveDir = _registrationAlgo.getSaveDirectory();

    std::ofstream defNormsFile;
    if ( _loadStageOneResults == false )
      defNormsFile.open ( ( origSaveDir + "/defNorms.txt" ).c_str() );

    std::ofstream energiesFile;
    energiesFile.open ( ( origSaveDir + "/energies.txt" ).c_str() );

    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    const int numTemplateImages = templateFileNames.size();
    const ArrayType reference ( _registrationAlgo.getRefImageReference(), aol::DEEP_COPY );
    const bool reverseRoles = _parser.checkAndGetBool ( "reverseRolesInSeriesMatching" );
    const bool calcInverseDeformation = _parser.checkAndGetBool ( "calcInverseDeformation" );
    if ( reverseRoles )
      _registrationAlgo.loadRefOrTemplate ( _parser.getString ( "reference" ).c_str(), qc::TEMPLATE, _parser.checkAndGetBool ( "dontNormalizeInputImages" ) );

    // Start with a clean deformation.
    _registrationAlgo.setTransformationToZero ( );
    TransformationDOFType accumulatedTransformation ( _registrationAlgo.getTransformationDOFInitializer() );
    // The "zero transformation" (aka the identity mapping) doesn't necessarily have zero DOFs.
    // So make sure it's initialized properly by using the clean deformation in _registrationAlgo.
    _registrationAlgo.getTransformation ( accumulatedTransformation );

    for ( int i = 0; i < numTemplateImages; ++i ) {
      // First match the current template with the last template (assuming that the first template is the reference image).

      // In case we try to reduce the deformations, we have to adjust the initial deformation used
      // for the first pairing.
      if ( _reduceDeformations && ( reverseRoles == false ) && ( i == 0 ) && ( _stage > 1 ) ) {
        _registrationAlgo.loadTransformation ( aol::strprintf ( "%s../stage%d/reduceDef_%%d.dat.bz2", _parser.getString ( "saveDirectory" ).c_str(), _stage - 1 ).c_str() );
        // In Stage 3+, this needs to be combined with the deformation calculated in the previous stage.
        if ( _stage > 2 ) {
          TransformationDOFType reducedTransformation ( _registrationAlgo.getTransformationDOFInitializer() );
          _registrationAlgo.getTransformation ( reducedTransformation );

          _registrationAlgo.loadTransformation ( aol::strprintf ( "%s../stage%d/0/deformation_%02d%s", _parser.getString ( "saveDirectory" ).c_str(), _stage-1, _registrationAlgo.getMaxGridDepth(), _registrationAlgo.getDeformationFileNameSuffix().c_str() ).c_str() );
          TransformationDOFType transformationToLastAverage ( _registrationAlgo.getTransformationDOFInitializer() );
          _registrationAlgo.getTransformation ( transformationToLastAverage );

          _registrationAlgo.setTransformationToComposition ( transformationToLastAverage, reducedTransformation );
        }

      }
      else // Start with a clean displacement (this pairing is new).
        _registrationAlgo.setTransformationToZero();

      _registrationAlgo.loadRefOrTemplate ( templateFileNames[i].c_str(), reverseRoles ? qc::REFERENCE : qc::TEMPLATE, _parser.checkAndGetBool ( "dontNormalizeInputImages" ) );
      if ( ( _loadStageOneResults == false ) || ( i == 0 ) ) {
        _registrationAlgo.makeAndSetSaveDirectory ( aol::strprintf ( "%s%d/", _parser.getString ( "saveDirectory" ).c_str(), i ).c_str() );
        _registrationAlgo.solveAndProlongToMaxDepth();
        if ( _loadStageOneResults == false )
          defNormsFile << i << " " << _registrationAlgo.getTransformationNorm () << " # " << templateFileNames[i] << endl;

        if ( _useAltStartLevel ) {
          const RealType firstTryEnergy = _registrationAlgo.getEnergyOfLastSolution();

          TransformationDOFType savedTransformation ( _registrationAlgo.getTransformationDOFInitializer() );
          _registrationAlgo.getTransformation ( savedTransformation );

          ArrayType temp ( _registrationAlgo.getInitializerRef() );
          _registrationAlgo.applyCurrentTransformation ( _registrationAlgo.getTemplImageReference(), temp );
          const int numOutOfDomainFirstTry = temp.numOccurence ( aol::NumberTrait<RealType>::Inf );

          _registrationAlgo.setTransformationToZero();
          _registrationAlgo.makeAndSetSaveDirectory ( aol::strprintf ( "%s%d-alt/", _parser.getString ( "saveDirectory" ).c_str(), i ).c_str() );
          _registrationAlgo.solveAndProlongToMaxDepth(  _parser.getInt ( "altStartLevel" ) );

          _registrationAlgo.applyCurrentTransformation ( _registrationAlgo.getTemplImageReference(), temp );
          const int numOutOfDomainSecondTry = temp.numOccurence ( aol::NumberTrait<RealType>::Inf );

          const bool firstTryBetterWRTDomain = ( numOutOfDomainFirstTry < numOutOfDomainSecondTry );
          const bool firstTryBetterWRTEnergy = ( firstTryEnergy < _registrationAlgo.getEnergyOfLastSolution() );
          const bool energyAsAltCriterion = _parser.checkAndGetBool( "energyAsAltCriterion" );

          cerr << "Using ";
          if ( ( energyAsAltCriterion && firstTryBetterWRTEnergy ) || ( !energyAsAltCriterion && firstTryBetterWRTDomain ) ) {
            cerr << "\"startLevel\"";
            _registrationAlgo.setTransformation ( savedTransformation );
          }
          else
            cerr << "\"altStartLevel\"";
          cerr << " result\n";
          cerr << ( firstTryBetterWRTDomain ? "\"startLevel\"" : "\"altStartLevel\"" ) << " has higher overlap\n";
          cerr << ( firstTryBetterWRTEnergy ? "\"startLevel\"" : "\"altStartLevel\"" ) << " has lower energy\n";

          _registrationAlgo.setLevel ( _registrationAlgo.getMaxGridDepth() );
          _registrationAlgo.saveTransformation ( aol::strprintf ( "%s/deformation-%03d%s", _parser.getString ( "saveDirectory" ).c_str(), i, _registrationAlgo.getDeformationFileNameSuffix().c_str() ).c_str() );
        }
        // The first deformation will not be refined, so save its energy directly.
        else if ( i == 0 )
          energiesFile << i << " " << _registrationAlgo.getEnergyOfLastSolution () << endl;
      }
      else {
        if ( _useAltStartLevel )
          _registrationAlgo.loadTransformation ( aol::strprintf ( "%s../stage1/deformation-%03d%s", _parser.getString ( "saveDirectory" ).c_str(), i-1, _registrationAlgo.getDeformationFileNameSuffix().c_str() ).c_str() );
        else
          _registrationAlgo.loadTransformation ( aol::strprintf ( "%s../stage1/%d/deformation_%02d%s", _parser.getString ( "saveDirectory" ).c_str(), i-1, _registrationAlgo.getMaxGridDepth(), _registrationAlgo.getDeformationFileNameSuffix().c_str() ).c_str() );
      }

      if ( _parser.checkAndGetBool ( "dontAccumulateDeformation" ) == false ) {
        _registrationAlgo.addTransformationTo ( accumulatedTransformation );

        // Now match the current template to the first template aka the reference.
        if ( i > 0 ) {
          // Start with the accumulated displacement.
          _registrationAlgo.setTransformation ( accumulatedTransformation );

          if ( reverseRoles )
            _registrationAlgo.setTemplate ( reference );
          else
            _registrationAlgo.setReference ( reference );

          _registrationAlgo.makeAndSetSaveDirectory ( aol::strprintf ( "%s%d-r/", _parser.getString ( "saveDirectory" ).c_str(), i ).c_str() );
          _registrationAlgo.solveAndProlongToMaxDepth ( _parser.getInt ( "refineStartLevel" ), _parser.getInt ( "refineStopLevel" ) );

          // Save the energy of the refined deformation.
          energiesFile << i << " " << _registrationAlgo.getEnergyOfLastSolution () << endl;

          // Store the refined transformation from the current template to the reference.
          _registrationAlgo.getTransformation ( accumulatedTransformation );
        }

        if ( calcInverseDeformation ) {
          const ArrayType savedTemplate ( _registrationAlgo.getTemplImageReference(), aol::DEEP_COPY );
          _registrationAlgo.setTemplate ( _registrationAlgo.getRefImageReference() );
          _registrationAlgo.setReference ( savedTemplate );
          TransformationDOFType inverseTransformation ( _registrationAlgo.getTransformationDOFInitializer() );
          qc::approxInverseDeformation<ConfiguratorType> ( _registrationAlgo.getInitializerRef(), accumulatedTransformation, inverseTransformation );
          _registrationAlgo.setTransformation ( inverseTransformation );
          _registrationAlgo.makeAndSetSaveDirectory ( aol::strprintf ( "%s%d-r-inv/", _parser.getString ( "saveDirectory" ).c_str(), i ).c_str() );
          _registrationAlgo.solveAndProlongToMaxDepth ( _parser.getInt ( "refineStartLevel" ), _parser.getInt ( "refineStopLevel" ) );
          _registrationAlgo.setReference ( _registrationAlgo.getTemplImageReference() );
          _registrationAlgo.setTemplate ( savedTemplate );
        }

        if ( reverseRoles )
          _registrationAlgo.setTemplate ( _registrationAlgo.getRefImageReference() );
        else
          _registrationAlgo.setReference ( _registrationAlgo.getTemplImageReference() );
      }
    }

    // We temporarily messed with the save directory. Restore the original value.
    _registrationAlgo.setSaveDirectory ( origSaveDir.c_str() );
  }

  void saveAverageMedianAndNumSamples ( const ArrayType &Average, const ArrayType &Median, const ArrayType &NumSamples, const int Iter = -1 ) const {
    qc::DefaultArraySaver<RealType, Dim> saver ( true );
    saver.setSaveDirectory ( _parser.getString ( "saveDirectory" ).c_str() );
    saver.saveStep ( Average, Iter, "average" );
    saver.saveStep ( Median, Iter, "median" );
    saver.saveStep ( NumSamples, Iter, "numSamples" );
  }

  void saveNamedDeformedTemplate ( const char *TemplateFileName, const ArrayType &DefTemplateArray ) const {
    qc::DefaultArraySaver<RealType, Dim> saver ( false, false, true );
    saver.setSaveDirectory ( _parser.getString ( "saveDirectory" ).c_str() );
    if ( _parser.checkAndGetBool ( "saveNamedDeformedDMXTemplatesAsDMX" )
        && ( aol::fileNameEndsWith( TemplateFileName, "dm3" ) || aol::fileNameEndsWith( TemplateFileName, "dm4" ) ) ) {
      qc::DM3Reader dmreader( TemplateFileName );
      dmreader.saveQuocDataInDM3Container ( DefTemplateArray, saver.createSaveName ( "", "", -1, aol::getBaseFileName( TemplateFileName ).c_str() ) );
    }
    else
      saver.saveStep ( DefTemplateArray, -1, aol::getBaseFileName( TemplateFileName ).c_str() );
  }

  void averageSeries ( const char *InputDirectory ) const {
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    const int numTemplateImages = templateFileNames.size();

    ArrayType curTemplate ( _registrationAlgo.getInitializerRef() );

    ArrayType average ( _registrationAlgo.getInitializerRef() );
    ArrayType median ( _registrationAlgo.getInitializerRef() );
    // Keep track of how many samples were available when averaging a certain pixel.
    ArrayType numSamples ( _registrationAlgo.getInitializerRef() );

    if ( _parser.checkAndGetBool ( "reverseRolesInSeriesMatching" ) == false ) {
      // In stage one, the first frame is used as reference and not deformed. Nevetheless,
      // save it if the user wants to save the deformed templates.
      if ( ( _stage == 1 ) && _parser.checkAndGetBool ( "saveNamedDeformedTemplates" ) ) {
        _registrationAlgo.loadAndPrepareImage ( _parser.getString ( "reference" ).c_str(), curTemplate, true, false, true );
        saveNamedDeformedTemplate ( _parser.getString ( "reference" ).c_str(), curTemplate );
      }

      aol::MultiVector<RealType> deformedTemplates ( numTemplateImages, _registrationAlgo.getInitializerRef().getNumberOfNodes() );
      aol::MultiVector<RealType> calculatedDeformedTemplates;

      const int averageSaveIncrement = _parser.hasVariable ( "averageSaveIncrement" ) ? _parser.getInt ( "averageSaveIncrement" ) : numTemplateImages;

      TransformationDOFType reducedPhi;
      if ( _reduceDeformations )
        reducedPhi.load ( aol::strprintf ( "%sreduceDef_%%d.dat.bz2", getSaveDirectory() ).c_str() );

      // First generate all deformed template images.
      for ( int i = 0; i < numTemplateImages; ++i ) {
        _registrationAlgo.loadAndPrepareImage ( templateFileNames[i].c_str(), curTemplate, true, false, true );
        const string defFileNameBase = createDeformationBaseFileName ( InputDirectory, i );
        if ( _reduceDeformations ) {
          _registrationAlgo.applyTransformation ( reducedPhi, curTemplate, average );
          _registrationAlgo.applySavedTransformation ( defFileNameBase.c_str(), average, deformedTemplates[i] );
        }
        else
          _registrationAlgo.applySavedTransformation ( defFileNameBase.c_str(), curTemplate, deformedTemplates[i] );
        calculatedDeformedTemplates.appendReference ( deformedTemplates[i] );
        if ( _parser.checkAndGetBool ( "saveNamedDeformedTemplates" ) ) {
          ArrayType defTemplateArray ( _registrationAlgo.getInitializerRef() );
          _registrationAlgo.applySavedTransformation ( defFileNameBase.c_str(), curTemplate, defTemplateArray, _parser.checkAndGetBool ( "saveNamedDeformedTemplatesExtendedWithMean" ) ? curTemplate.getMeanValue() : 0, _parser.checkAndGetBool ( "saveNamedDeformedTemplatesUsingNearestNeighborInterpolation" ) );
          saveNamedDeformedTemplate ( templateFileNames[i].c_str(), defTemplateArray );
        }

        if ( ( calculatedDeformedTemplates.numComponents() == numTemplateImages ) || ( ( calculatedDeformedTemplates.numComponents() % averageSaveIncrement ) == 0 ) ) {
          const RealType lastTemplateMean = curTemplate.getMeanValue ();

          // Now calculate the average and median.
          for ( int j = 0; j < average.size(); ++j ) {
            average[j] = 0;
            int numSamplesInDomain = 0;
            for ( int i = 0; i < calculatedDeformedTemplates.numComponents(); ++i ) {
              if ( calculatedDeformedTemplates[i][j] != aol::NumberTrait<RealType>::Inf ) {
                average[j] += calculatedDeformedTemplates[i][j];
                ++numSamplesInDomain;
              }
            }
            if ( numSamplesInDomain > 0 )
              average[j] /= numSamplesInDomain;
            else
              average[j] = lastTemplateMean;

            numSamples[j] = numSamplesInDomain;
          }

          calculatedDeformedTemplates.getMedianVecOverComponents ( median, lastTemplateMean );
          saveAverageMedianAndNumSamples ( average, median, numSamples, ( calculatedDeformedTemplates.numComponents() != numTemplateImages ) ? calculatedDeformedTemplates.numComponents() : -1 );
        }
      }
    }
    else {
      qc::MultiArray<RealType, Dim> phi ( _registrationAlgo.getInitializerRef() );
      qc::GridSize<Dim> targetSize ( _registrationAlgo.getInitializerRef() );
      const int factor = _parser.getIntOrDefault ( "reverseRolesSRFactor", 1 );
      targetSize *= factor;
      const qc::GridStructure targetGrid ( targetSize );
      if ( factor != 1 ) {
        numSamples.reallocate ( targetGrid );
        average.reallocate ( targetGrid );
        median.reallocate ( targetGrid );
      }
      qc::AArray<aol::Vector<RealType>, Dim> samples ( targetGrid );
      qc::AArray<aol::Vector<RealType>, Dim> weights ( targetGrid );
      const RealType postionDistanceThreshold = _parser.hasVariable ( "reverseRolesPostionDistanceThreshold" ) ? _parser.getReal<RealType> ( "reverseRolesPostionDistanceThreshold" ) : 1;
      const bool reverseRolesWeightMedianAndMean = _parser.checkAndGetBool ( "reverseRolesWeightMedianAndMean" );
      // The code used to calculate middlePointPos makes the assumption that the distance
      // from exactDeformedPos to deformedPos is big enough.
      if ( postionDistanceThreshold < 0.251 )
        throw aol::Exception ( "SeriesMatching::averageSeries: Invalid reverseRolesPostionDistanceThreshold value", __FILE__, __LINE__ );

      aol::ProgressBar<> progressBar ( "Sampling data" );
      progressBar.start ( numTemplateImages );
      progressBar.display ( cerr );
      // First generate all deformed template images.
      for ( int i = 0; i < numTemplateImages; ++i ) {
        _registrationAlgo.loadAndPrepareImage ( templateFileNames[i].c_str(), curTemplate, true );
        const string defFileNameBase = createDeformationFileName ( InputDirectory, i );
        phi.load ( defFileNameBase.c_str() );

        for ( qc::RectangularIterator<Dim> it ( _registrationAlgo.getInitializerRef() ); it.notAtEnd(); ++it ) {
          typename ConfiguratorType::VecType exactDeformedPos;
          qc::CoordType deformedPos;
          typename ConfiguratorType::VecType distToExactDeformedPos;
          for ( int j = 0; j < Dim; ++j ) {
            exactDeformedPos[j] = ( (*it)[j] + phi[j].get ( *it ) / _registrationAlgo.getInitializerRef().H() ) * factor;
            deformedPos[j] = reverseRolesWeightMedianAndMean ? static_cast<short> ( std::floor ( exactDeformedPos[j] ) ) : aol::Rint ( exactDeformedPos[j] );
            distToExactDeformedPos[j] = exactDeformedPos[j] - deformedPos[j];
          }
          if ( reverseRolesWeightMedianAndMean ) {
            // The following only works in 2D.
            for ( int k = 0; k <= 1; ++k ) {
              const RealType xWeight = ( k == 0 ) ? ( 1 - distToExactDeformedPos[0] ) : ( distToExactDeformedPos[0] );
              if ( xWeight > 0 ) {
                qc::CoordType node;
                node[0] = deformedPos[0] + k;
                for ( int l = 0; l <= 1; ++l ) {
                  const RealType yWeight = ( l == 0 ) ? ( 1 - distToExactDeformedPos[1] ) : ( distToExactDeformedPos[1] );
                  if ( yWeight > 0 ) {
                    node[1] = deformedPos[1] + l;
                    if ( targetGrid.isAdmissibleNode ( node ) )
                    {
                      samples.getRef ( node ).pushBack ( curTemplate.get ( *it ) );
                      weights.getRef ( node ).pushBack ( xWeight * yWeight );
                    }
                  }
                }
              }
            }
          }
          else if ( distToExactDeformedPos.getMinAbsValue() > postionDistanceThreshold ) {
            typename ConfiguratorType::VecType middlePointPos;
            for ( int j = 0; j < Dim; ++j )
              middlePointPos[j] = aol::Rint ( 2 * distToExactDeformedPos[j] ) * 0.5 + deformedPos[j];
            // The following only works in 2D.
            for ( int k = -1; k <= 1; k = k + 2 ) {
              deformedPos[0] = aol::Rint ( middlePointPos[0] + k * 0.5 );
              for ( int l = -1; l <= 1; l = l + 2 ) {
                deformedPos[1] = aol::Rint ( middlePointPos[1] + l * 0.5 );
                if ( targetGrid.isAdmissibleNode ( deformedPos ) )
                  samples.getRef ( deformedPos ).pushBack ( curTemplate.get ( *it ) );
              }
            }
          }
          else if ( targetGrid.isAdmissibleNode ( deformedPos ) ) {
            samples.getRef ( deformedPos ).pushBack ( curTemplate.get ( *it ) );
          }
        }
        progressBar++;
      }
      progressBar.finish();

      // Take the value of the reference image as sample for those pixels where we don't have any samples yet.
      {
        ArrayType referenceArray ( _registrationAlgo.getInitializerRef() );
        _registrationAlgo.loadAndPrepareImage ( _parser.getString ( "reference" ).c_str(), referenceArray, true );

        for ( qc::RectangularIterator<Dim> it ( samples ); it.notAtEnd(); ++it ) {
          if ( samples.getRef ( *it ).size() == 0 ) {
            if ( factor == 1 )
              samples.getRef ( *it ).pushBack ( referenceArray.get ( *it ) );
            else {
              aol::Vec<Dim, RealType> pos;
              for ( int j = 0; j < Dim; ++j )
                pos[j] = aol::Min ( (*it)[j] / static_cast<RealType> ( factor ), static_cast<RealType> ( referenceArray.getSize()[j] - 1 ) );

              samples.getRef ( *it ).pushBack ( referenceArray.interpolate ( pos ) );
            }
            if ( reverseRolesWeightMedianAndMean )
              weights.getRef ( *it ).pushBack ( 1 );
          }
        }
      }

      for ( int i = 0; i < average.size(); ++i ) {
        numSamples[i] = samples[i].size();
        average[i] = reverseRolesWeightMedianAndMean ? samples[i].getWeightedMeanValue ( weights[i] ) : samples[i].getMeanValue();
        median[i] = reverseRolesWeightMedianAndMean ? samples[i].getWeightedMedianValue( weights[i] ) : samples[i].getMedianValue();
      }

      saveAverageMedianAndNumSamples ( average, median, numSamples );
    }
  }

  void matchAndAverageSeries ( ) {
    matchSeries();
    if ( _reduceDeformations )
      reduceDeformations ( _registrationAlgo.getSaveDirectory() );
    averageSeries ( _registrationAlgo.getSaveDirectory() );
  }

  void analyzeDeformations ( const char *InputDirectory ) {
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    const int numDeformations = templateFileNames.size();

    ArrayType temp ( _registrationAlgo.getInitializerRef() );
    qc::MultiArray<RealType, Dim> phi ( _registrationAlgo.getInitializerRef() );

    aol::RandomAccessContainer<aol::Vec<Dim, RealType> > translations ( numDeformations );

    RealType averageNonRigidComponentMax = 0;
    RealType averageNonRigidComponent = 0;

    for ( int i = 0; i < numDeformations; ++i ) {
      phi.load ( createDeformationFileName ( InputDirectory, i ).c_str() );

      if ( _parser.checkAndGetBool ( "saveDeformedTemplates" ) ) {
        ArrayType curTemplate ( _registrationAlgo.getInitializerRef() );
        _registrationAlgo.loadAndPrepareImage ( templateFileNames[i].c_str(), curTemplate, true );
        _registrationAlgo.applyTransformation ( phi, curTemplate, temp, aol::NumberTrait<RealType>::Inf, _parser.checkAndGetBool ( "saveDeformedTemplatesUsingNearestNeighborInterpolation" ) );

        qc::DefaultArraySaver<RealType, Dim> saver ( true, true );
        saver.setSaveDirectory ( getSaveDirectory() );
        saver.setSaveTimestepOffset ( 1 );

        if ( _parser.hasVariable ( "enhanceContrastSaturationPercentage" ) )
          saver.setEnhanceContrastSaturationPercentage ( _parser.getDouble ( "enhanceContrastSaturationPercentage" ) );

        temp.save( saver.createSaveName ( "", ".dat.bz2", i, "defTempl" ).c_str(), qc::PGM_DOUBLE_BINARY );

        if ( Dim == qc::QC_2D ) {
          // Replace Inf by the mean when saving as PNG.
          const RealType curTemplateMean = curTemplate.getMeanValue();
          for ( int j = 0; j < curTemplate.size(); ++j ) {
            if ( aol::isInf ( temp[j] ) )
              temp[j] = curTemplateMean;
          }
          saver.saveStep ( temp, i, "defTempl" );
        }
      }

      translations[i] = phi.getMeanValue();

      for ( int c = 0; c < Dim; ++c )
        phi[c].addToAll ( -translations[i][c] );

      qc::ScalarArray<RealType, Dim> phiNorms ( _registrationAlgo.getInitializerRef() );
      phi.getPointWiseNorm( phiNorms );
      const RealType phiMaxOfPointWiseNorm = phiNorms.getMaxValue();
      averageNonRigidComponentMax += phiMaxOfPointWiseNorm;
      averageNonRigidComponent += phiNorms.getMeanValue();

      cerr << "Translation " << translations[i] << ", non-rigid component max " << phiMaxOfPointWiseNorm / translations[i].norm() * 100 << "%, " << phiMaxOfPointWiseNorm / _registrationAlgo.getInitializerRef().H() << " pixels" <<endl;

      if ( _parser.checkAndGetBool ( "saveXDerivativeOfDef" ) ) {
        for ( qc::RectangularIterator<Dim> it ( _registrationAlgo.getInitializerRef() ); it.notAtEnd(); ++it ) {
          aol::Vec<ConfiguratorType::Dim, RealType> tempVec;
          for ( int comp = 0; comp < ConfiguratorType::Dim; ++comp )
            tempVec[comp] = phi[comp].dxFD( *it );
          temp.set ( *it, tempVec.norm() );
        }
        qc::DefaultArraySaver<RealType, Dim> saver ( true, true );
        saver.setSaveDirectory ( getSaveDirectory() );
        saver.setSaveTimestepOffset ( 1 );
        saver.saveStep ( temp, i, "def" );
      }
    }

    averageNonRigidComponentMax /= numDeformations;
    averageNonRigidComponent /= numDeformations;

    RealType averageTranslation = 0;
    for ( int i = 1; i < numDeformations; ++i ) {
      averageTranslation += ( translations[i] - translations[i-1] ).norm();
    }
    averageTranslation /= ( numDeformations - 1 );
    cerr << "average translation = " << averageTranslation << ", " << averageTranslation / _registrationAlgo.getInitializerRef().H() << " pixels" << endl;
    cerr << "average non-rigid component = " << averageNonRigidComponent << ", " << averageNonRigidComponent / _registrationAlgo.getInitializerRef().H() << " pixels" << endl;
    cerr << "ratio  = " << averageNonRigidComponent / averageTranslation << endl;
    cerr << "average non-rigid component max = " << averageNonRigidComponentMax << ", " << averageNonRigidComponentMax / _registrationAlgo.getInitializerRef().H() << " pixels" << endl;
    cerr << "ratio  = " << averageNonRigidComponentMax / averageTranslation << endl;

    aol::PlotDataFileHandler<RealType> plotDataFileHandler;
    plotDataFileHandler.generateCurvePlot ( translations );
    aol::Plotter<RealType> plotter;
    plotter.addPlotCommandsFromHandler( plotDataFileHandler );
    plotter.set_outfile_base_name ( aol::strprintf ( "%strans", getSaveDirectory() ).c_str() );
    plotter.genPlot( aol::GNUPLOT_PNG );

    plotDataFileHandler.clear();
    plotter.clearAdditionalPlotCommands();
    aol::Vector<RealType> translationNorms ( numDeformations );
    for ( int i = 0; i < translations.size(); ++i )
      translationNorms[i] = translations[i].norm();
    plotDataFileHandler.generateFunctionPlot ( translationNorms, 1, translations.size() );
    plotter.addPlotCommandsFromHandler( plotDataFileHandler );
    plotter.set_outfile_base_name ( aol::strprintf ( "%snorms", getSaveDirectory() ).c_str() );
    plotter.genPlot( aol::GNUPLOT_PNG );
  }

  void reduceDeformations ( const char *InputDirectory ) {
    const int numDeformations = getNumTemplates();

    aol::RandomAccessContainer< qc::MultiArray<RealType, Dim> > deformations ( numDeformations, _registrationAlgo.getInitializerRef() );
    aol::RandomAccessContainer< const qc::ConsistencyEnergyOp<ConfiguratorType> > energies;
    aol::RandomAccessContainer< const qc::VariationOfConsistencyEnergyOp<ConfiguratorType> > derivatives;

    aol::LinCombOp<aol::MultiVector<RealType>, aol::Scalar<RealType> > E;
    aol::LinCombOp<aol::MultiVector<RealType> > DE;

    for ( int i = 0; i < numDeformations; ++i ) {
      deformations[i].load ( createDeformationFileName ( InputDirectory, i ).c_str() );
      energies.constructDatumAndPushBack ( _registrationAlgo.getInitializerRef(), deformations[i] );
      E.appendReference ( energies[i] );
      derivatives.constructDatumAndPushBack ( _registrationAlgo.getInitializerRef(), deformations[i] );
      DE.appendReference ( derivatives[i] );
    }

    typedef aol::H1GradientDescent<ConfiguratorType, aol::MultiVector<RealType>, qc::LinearSmoothOp<RealType, typename qc::MultilevelArrayTrait<RealType, typename ConfiguratorType::InitType>::GridTraitType > > GDType;
    GDType solver ( _registrationAlgo.getInitializerRef(), E, DE, 1000, 1, 5e-7 );
    solver.setConfigurationFlags ( GDType::USE_NONLINEAR_CG|GDType::LOG_GRADIENT_NORM_AT_OLD_POSITION|GDType::USE_GRADIENT_BASED_STOPPING );

    qc::MultiArray<RealType, Dim> phi ( _registrationAlgo.getInitializerRef() );
    solver.applySingle ( phi );
    phi.save ( aol::strprintf ( "%sreduceDef_%%d.dat.bz2", getSaveDirectory() ).c_str(), qc::PGM_DOUBLE_BINARY );
  }

  void applyDeformation ( ) {
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    const int numTemplateImages = templateFileNames.size();

    if ( _stage == 1 ) {
      std::vector<std::string>::iterator it;
      it = templateFileNames.begin();
      it = templateFileNames.insert ( it , _parser.getString ( "reference" ) );
    }

    ArrayType curTemplate ( _registrationAlgo.getInitializerRef() );
    ArrayType defTemplateArray ( _registrationAlgo.getInitializerRef() );
    const int numDeformations = _parser.getInt ( "numDeformations" );
    TransformationDOFType deformationDofs ( _registrationAlgo.getTransformationDOFInitializer() );
    _registrationAlgo.loadTransformationTo ( aol::strprintf ( _parser.getString ( "deformationBaseNamePattern" ).c_str(), numDeformations - 1, numDeformations - 1 ).c_str(), deformationDofs );

    for ( int i = numDeformations - 2; i >= 0; --i ) {
      _registrationAlgo.loadTransformation ( ( aol::strprintf ( _parser.getString ( "deformationBaseNamePattern" ).c_str(), i, i ) + _registrationAlgo.getDeformationFileNameSuffix() ).c_str() );
      _registrationAlgo.addTransformationTo ( deformationDofs );
    }

    for ( int i = 0; i < numTemplateImages; ++i ) {
      _registrationAlgo.loadAndPrepareImage ( templateFileNames[i].c_str(), curTemplate, true, false, true );

      _registrationAlgo.applyTransformation ( deformationDofs, curTemplate, defTemplateArray, _parser.checkAndGetBool ( "saveNamedDeformedTemplatesExtendedWithMean" ) ? curTemplate.getMeanValue() : 0, _parser.checkAndGetBool ( "saveNamedDeformedTemplatesUsingNearestNeighborInterpolation" ) );
      saveNamedDeformedTemplate ( templateFileNames[i].c_str(), defTemplateArray );
    }
  }

  void discardBrokenFramesFromSeries ( ) const {
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );
    const int numTemplateImages = templateFileNames.size();

    ArrayType currentFrame ( templateFileNames[0] );
    const aol::Vector<RealType> lastLineOfCurrentFrame ( currentFrame.getData(), currentFrame.getNumX(), aol::FLAT_COPY );

    int numGoodTemplates = 1;
    currentFrame.save ( aol::strprintf ( "%sgood-%03d.dat.bz2", getSaveDirectory(), numGoodTemplates-1 ).c_str(), qc::PGM_DOUBLE_BINARY );

    for ( int frameNum = 1; frameNum < numTemplateImages; ++frameNum ) {
      const ArrayType nextFrame ( templateFileNames[frameNum] );
      const aol::Vector<RealType> lastLineOfNextFrame ( nextFrame.getData(), nextFrame.getNumX(), aol::FLAT_COPY );

      if ( lastLineOfCurrentFrame != lastLineOfNextFrame ) {
        currentFrame = nextFrame;
        ++numGoodTemplates;
        currentFrame.save ( aol::strprintf ( "%sgood-%03d.dat.bz2", getSaveDirectory(), numGoodTemplates-1 ).c_str(), qc::PGM_DOUBLE_BINARY );
      }
    }
  }

  void calcMedianOfUnregistredTemplates ( ) const {
    std::vector<std::string> templateFileNames;
    createTemplateFileNameList ( templateFileNames );

    const int numTemplateImages = templateFileNames.size();

    ArrayType median ( _registrationAlgo.getInitializerRef() );

    aol::MultiVector<RealType> templates ( numTemplateImages, _registrationAlgo.getInitializerRef().getNumberOfNodes() );

    for ( int i = 0; i < numTemplateImages; ++i ) {
      ArrayType templateArray ( templates[i], _registrationAlgo.getInitializerRef(), aol::FLAT_COPY );
      _registrationAlgo.loadAndPrepareImage ( templateFileNames[i].c_str(), templateArray, true );
    }

    templates.getMedianVecOverComponents ( median );
    median.save ( aol::strprintf ( "%smedianOfTemplates.dat.bz2", getSaveDirectory() ).c_str(), qc::PGM_DOUBLE_BINARY );
  }

  const char* getSaveDirectory () const {
    return _registrationAlgo.getSaveDirectory();
  }

  void doAction ( const ACTION ActionType ) {
    switch ( ActionType ) {
      case MATCH_AND_AVERAGE_SERIES :
        matchAndAverageSeries();
        break;
      case ONLY_AVERAGE_SERIES:
        averageSeries( getSaveDirectory() );
        break;
      case ANALYZE_DEFORMATTIONS:
        analyzeDeformations( getSaveDirectory() );
        break;
      case APPLY_DEFORMATTION:
        applyDeformation( );
        break;
      default:
        throw aol::Exception ( "SeriesMatching::doAction: Invalid ActionType", __FILE__, __LINE__ );
    } 
  }
};

#endif // __MATCHSERIES_H

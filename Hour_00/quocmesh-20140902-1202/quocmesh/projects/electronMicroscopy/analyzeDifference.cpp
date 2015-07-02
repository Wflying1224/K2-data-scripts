#include <scalarArray.h>
#include <parameterParser.h>
#include <quocTimestepSaver.h>
#include <patchSet.h>

typedef double RType;

template <typename RealType>
void analyzeImage ( const qc::ScalarArray<RType, qc::QC_2D> &Image, const int PatchSize, qc::ScalarArray<RType, qc::QC_2D> &ImageSumsAbs ) {
  ImageSumsAbs.reallocate ( qc::GridSize2d ( Image.getNumX() - PatchSize + 1, Image.getNumY() - PatchSize + 1 ) );
  qc::ScalarArray<RealType, qc::QC_2D> temp ( PatchSize, PatchSize );

  aol::ProgressBar<> pb ( "Analyze image" );
  pb.start ( ImageSumsAbs.size() );
  for ( qc::RectangularIterator<qc::QC_2D, aol::Vec2<short> > it ( ImageSumsAbs ); it.notAtEnd(); ++it ) {
    Image.copyBlockTo ( *it, temp );
    ImageSumsAbs.set ( *it, aol::Abs ( temp.getMeanValue() ) );
    pb++;
  }
  pb.finish();
}

int main( int argc, char **argv ) {
  try {

    if ( argc > 2 ) {
      cerr << "USAGE: " << argv[0] << " <parameterfile>" << endl;
      return EXIT_FAILURE;
    }

    aol::ParameterParser parser ( argc, argv, "analyzeDifference.par" );

    qc::ScalarArray<RType, qc::QC_2D> difference ( parser.getString( "imageA" ) );
    const qc::ScalarArray<RType, qc::QC_2D> imageB ( parser.getString( "imageB" ) );
    difference -= imageB;
    if ( parser.checkAndGetBool ( "cropInput" ) )
      difference.crop ( parser.getIntVec<2> ( "cropStart" ), parser.getIntVec<2> ( "cropSize" ) );

    const int patchSizeStart = parser.getInt( "patchSizeStart" );
    const int patchSizeStop = parser.getInt( "patchSizeStop" );

    qc::DefaultArraySaver<RType, qc::QC_2D> saver ( true );
    saver.setAndCreateSaveDirectory ( aol::strprintf( "%s-%s-%s", parser.getString( "saveDirectory" ).c_str(),
                                                                  aol::getBaseFileName ( parser.getString( "imageA" ) ).c_str(),
                                                                  aol::getBaseFileName ( parser.getString( "imageB" ) ).c_str() ).c_str() );
    saver.setSaveName ( "diff" );
    saver.setSaveTimestepOffset ( 1 );
    saver.setStepDigitsFromMaxIter ( patchSizeStop );
    difference.save ( saver.createSaveName ( "", "-signed.dat.bz2", -1, NULL ).c_str(), qc::PGM_DOUBLE_BINARY );

    qc::DefaultArraySaver<RType, qc::QC_2D> diffSumsAbsMeanValsSaver;
    diffSumsAbsMeanValsSaver.copyFrom ( saver );
    diffSumsAbsMeanValsSaver.setOuterStepDigitsFromMaxIter ( patchSizeStop );

    std::ofstream outtxt ( saver.createSaveName ( "", ".txt" ).c_str() );

    qc::ScalarArray<RType, qc::QC_2D> diffSumsAbs;
    for ( int patchSize = patchSizeStart; patchSize <= patchSizeStop; ++++patchSize ) {
      analyzeImage<RType> ( difference, patchSize, diffSumsAbs );

      outtxt << patchSize << " " << diffSumsAbs.getMeanValue() << endl;

      diffSumsAbsMeanValsSaver.setDefaultOuterIteration ( patchSize );
      for ( int i = 2; i < 20; ++i ) {
        qc::PatchSet2D<RType, 1> diffSumsAbsPatches ( aol::Min ( diffSumsAbs.getNumX(), diffSumsAbs.getNumY() ) / i );
        diffSumsAbsPatches.extractPatchesFromArray ( diffSumsAbs, false );
        qc::ScalarArray<RType, qc::QC_2D> diffSumsAbsMeanVals ( diffSumsAbsPatches.getSize() );
        for ( qc::RectangularIterator<qc::QC_2D, aol::Vec2<short> > it ( diffSumsAbsPatches ); it.notAtEnd(); ++it )
          diffSumsAbsMeanVals.set ( *it, diffSumsAbsPatches.getPatch ( *it ).getMeanValue() );
        diffSumsAbsMeanVals.saveASCII ( diffSumsAbsMeanValsSaver.createSaveName ( "", ".dat", i ).c_str(), aol::shortFormat );
        diffSumsAbsMeanVals.save ( diffSumsAbsMeanValsSaver.createSaveName ( "", ".dat.bz2", i, NULL ).c_str(), qc::PGM_DOUBLE_BINARY );

        if ( ( ( patchSize == 5 ) || ( patchSize == 15 ) ) && ( i == 9 ) ) {
          analyzeImage<RType> ( diffSumsAbs, diffSumsAbsPatches.getPatchWidth(), diffSumsAbsMeanVals );
          diffSumsAbsMeanVals.saveASCII ( diffSumsAbsMeanValsSaver.createSaveName ( "", "full.dat", i ).c_str(), aol::shortFormat );
          diffSumsAbsMeanVals.save ( diffSumsAbsMeanValsSaver.createSaveName ( "", "full.dat.bz2", i, NULL ).c_str(), qc::PGM_DOUBLE_BINARY );
        }
      }

      saver.saveStep ( diffSumsAbs, patchSize );
      diffSumsAbs.setOverflowHandling ( aol::CLIP_THEN_SCALE, 0, 255. / patchSize );
      diffSumsAbs.savePNG ( saver.createSaveName ( "", ".png", patchSize ).c_str() );

      qc::ScalarArray<RType, qc::QC_2D> modulus ( diffSumsAbs, aol::STRUCT_COPY );
      qc::computeLogFFTModulus<RType> ( diffSumsAbs, modulus, 0.7 );
      modulus.setOverflowHandling ( aol::CLIP_THEN_SCALE, modulus.getMinValue(), modulus.getMaxValue() );
      modulus.savePNG ( saver.createSaveName ( "", ".png", patchSize, ( string ( saver.getSaveName() ) + "FFT" ).c_str() ).c_str() );
    }

    outtxt.close();
  }
  catch ( aol::Exception &el ) {
    el.dump();
  }
  aol::callSystemPauseIfNecessaryOnPlatform();
  return 0;
}

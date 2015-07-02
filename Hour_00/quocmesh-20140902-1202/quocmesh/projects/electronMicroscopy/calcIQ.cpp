#include <scalarArray.h>
#include <parameterParser.h>
#include <quocTimestepSaver.h>
#include <patchSet.h>

typedef double RType;

template <typename RealType>
void findMaxima ( const qc::ScalarArray<RType, qc::QC_2D> &Image, const int PatchSize, aol::RandomAccessContainer<aol::Vec2<short> > &Maxima ) {
  const int offset = PatchSize/2;
  const qc::CoordType lower ( PatchSize, PatchSize );
  const qc::CoordType upper ( Image.getNumX() - 2*PatchSize + 1, Image.getNumY() - 2*PatchSize + 1 );
  qc::ScalarArray<RealType, qc::QC_2D> temp ( PatchSize, PatchSize );
  for ( qc::RectangularIterator<qc::QC_2D> it ( lower, upper ); it.notAtEnd(); ++it ) {
    Image.copyBlockTo ( *it, temp );
    if ( temp.get ( offset, offset ) >= temp.getMaxValue() )
      Maxima.pushBack ( aol::Vec2<short> ( (*it)[0] + offset, (*it)[1] + offset ) );
  }
}

int main( int argc, char **argv ) {
  try {

    if ( argc > 2 ) {
      cerr << "USAGE: " << argv[0] << " <parameterfile>" << endl;
      return EXIT_FAILURE;
    }

    aol::ParameterParser parser ( argc, argv, "calcIQ.par" );

    const int patchSize = parser.getInt( "patchSize" );
    const RType threshold = parser.getReal<RType>( "threshold" );
    const RType onePixelInAngstrom = parser.getReal<RType>( "onePixelInAngstrom" );
    const qc::ScalarArray<RType, qc::QC_2D> image ( parser.getString( "image" ) );
    /*
    qc::ScalarArray<RType, qc::QC_2D> image ( 1024, 1024 );
    image.setAll( imageOrig.getMeanValue() );
    imageOrig.copyBlockTo ( aol::Vec2<short> ( 0, 0 ), image );
    */
    qc::ScalarArray<RType, qc::QC_2D> modulus ( image, aol::STRUCT_COPY );
    qc::computeLogFFTModulus<RType> ( image, modulus, 0, false );
    aol::RandomAccessContainer<aol::Vec2<short> > maxima;
    findMaxima<RType> ( modulus, patchSize, maxima );

    std::vector<std::pair<RType, RType> > IQFactors;
    qc::ScalarArray<RType, qc::QC_2D> maximaImage ( image, aol::STRUCT_COPY );
    qc::ScalarArray<RType, qc::QC_2D> temp ( patchSize, patchSize );
    const RType hx = aol::ZOTrait<RType>::one / ( image.getNumX() - 1 );
    const RType hy = aol::ZOTrait<RType>::one / ( image.getNumY() - 1 );
    for ( int i = 0; i < maxima.size(); ++i ) {
      aol::Vector<RType> backgroundValues;
      backgroundValues.reserve ( 4 );
      for ( int k = -1; k <= 1; k = k + 2 ) {
        for ( int l = -1; l <= 1; l = l + 2 ) {
          modulus.copyBlockTo ( aol::Vec2<short> ( maxima[i][0] - patchSize/2 + k * patchSize, maxima[i][1] - patchSize/2 + l * patchSize ), temp );
          backgroundValues.pushBack ( temp.getMeanValue() );
        }
      }
      const RType IQ = modulus.get ( maxima[i] ) / backgroundValues.getMeanValue();
      if ( IQ > threshold ) {
        IQFactors.push_back ( std::pair<RType, RType> ( aol::Vec2<RType> ( ( image.getNumX() / 2 - maxima[i][0] ) * hx / onePixelInAngstrom, ( image.getNumY() / 2 - maxima[i][1] ) * hy / onePixelInAngstrom ).norm(), IQ ) );
        maximaImage.set ( maxima[i], 255 );
      }
    }
    std::sort( IQFactors.begin(), IQFactors.end() );

    qc::DefaultArraySaver<RType, qc::QC_2D> saver ( true, true );
    saver.setAndCreateSaveDirectory ( aol::strprintf ( "%s-%s", parser.getString( "saveDirectory" ).c_str(), aol::getBaseFileName ( parser.getString( "image" ) ).c_str() ).c_str() );

    aol::Plotter<RType> plotter;
    plotter.set_outfile_base_name( saver.createSaveName ( "", "", -1, "IQplot" ) );
    aol::PlotDataFileHandler<RType> plotHandler;
    plotHandler.generateFunctionPlot( IQFactors );
    plotter.addPlotCommandsFromHandler( plotHandler );
    plotter.genPlot( aol::GNUPLOT_PNG );

    std::ofstream outdat ( saver.createSaveName ( "", ".dat", -1, "IQplot" ).c_str() );
    for ( unsigned int i = 0; i < IQFactors.size(); ++i )
      outdat << IQFactors[i].first << " " << IQFactors[i].second << endl;
    outdat.close();

    saver.saveStep ( maximaImage, -1, "maxima" );
    saver.saveStep ( modulus, -1, "modulus" );
  }
  catch ( aol::Exception &el ) {
    el.dump();
  }
  aol::callSystemPauseIfNecessaryOnPlatform();
  return 0;
}

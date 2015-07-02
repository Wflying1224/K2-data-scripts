/**
 * \file
 * \brief Converts a DM3 file to a ScalarArray with double precision. If the DM3 file contains multiple frames, the output are separate 2D arrays.
 *
 * Usage: convertDM3ToQuoc InputFile
 *
 * \author Berkels
 */

#include <dm3Import.h>

int main ( int argc, char **argv ) {

  try {
    if ( argc < 2 ) {
      cerr << "USAGE: " << argv[0] << "  <InputFile>" << endl;
      return EXIT_FAILURE;
    }

    const string inFileName = argv[1];
    qc::DM3Reader dmreader( inFileName );
    dmreader.saveDataAsScalarArray ( aol::getBaseFileName( inFileName ), true );

  }//try
  catch ( aol::Exception &el ) {
    el.dump();
  }
  aol::callSystemPauseIfNecessaryOnPlatform();
  return 0;
}

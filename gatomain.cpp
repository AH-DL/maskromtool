#include <QCoreApplication>
#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QByteArray>
#include <QRandomGenerator>

#include <QPrintDialog>
#include <QPrinter>

#include <iostream>
#include <string>
#include <stdio.h>

#include "gatorom.h"
#include "gatosolver.h"

//Custom decoders.
#include "gatodecoderinfo.h"
#include "gatodecoderarm6.h"  //MYK82 Fortezza
#include "gatodecodermsp430.h"
#include "gatodecodertlcsfont.h"
#include "gatodecodercolsdownlswap.h" // NEC uCOM4

//Zorrom compatibility.
#include "gatodecodercolsdownr.h"
#include "gatodecodercolsdownl.h"
#include "gatodecodercolsleft.h"
#include "gatodecodercolsright.h"
#include "gatodecodersqueezelr.h"

//Graders for the solver.
#include "gatograderbytes.h"
#include "gatograderstring.h"


/* This is a quick CLI wrapper for GatoROM, which you might run on textfiles
 * exported from MaskROMTool, Bitract or Rompar.
 */

int main(int argc, char *argv[]) {
    //QCoreApplication a(argc, argv);
    QApplication a(argc,argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Gato ROM: A Decoder for Mask ROM Bits");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("bitstream", "ASCII art of ROM to decode.");


    //Rotations come first.
    QCommandLineOption rotateOption(QStringList()<<"r"<<"rotate",
                                    "Rotates the image in multiples of 90 degrees.",
                                    "degrees",
                                    "0"
                                    );
    parser.addOption(rotateOption);
    //Then flips
    QCommandLineOption flipxOption(QStringList()<<"flipx",
                                    "Flips the bits along the X axis."
                                    );
    parser.addOption(flipxOption);
    QCommandLineOption flipyOption(QStringList()<<"flipy",
                                    "Flips the bits along the Y axis."
                                    );
    parser.addOption(flipyOption);
    //And transformations
    QCommandLineOption invertOption(QStringList()<<"i"<<"invert",
                                    "Inverts the bits."
                                    );
    parser.addOption(invertOption);


    //Output file for the decoder.
    QCommandLineOption outputOption(QStringList()<<"o"<<"output",
                                    "Output file.",
                                    "out.bin"
                                    );
    parser.addOption(outputOption);

    //Randomizing without an input file.
    QCommandLineOption randomOption(QStringList()<<"random",
                                    "Randomize a ROM."
                                    );
    parser.addOption(randomOption);

    //Randomizing without an input file.
    QCommandLineOption seanriddleOption(QStringList()<<"rawwidth"<<"seanriddle",
                                    "Width of a raw binary input, in Sean Riddle's style.",
                                    "width"
                                    );
    parser.addOption(seanriddleOption);


    //Info about the ROM.
    QCommandLineOption infoOption(QStringList()<<"I"<<"info",
                                    "Info about input."
                                    );
    parser.addOption(infoOption);

    //Print with a dialog.
    QCommandLineOption printOption(QStringList()<<"print",
                                   "Print with a GUI dialog."
                                   );
    parser.addOption(printOption);
    //Print to a file.
    QCommandLineOption printpdfOption(QStringList()<<"printpdf",
                                   "Print to a PDF file.",
                                   "file.pdf"
                                   );
    parser.addOption(printpdfOption);


    //Decoder itself.
    QCommandLineOption arm6Option(QStringList()<<"decode-arm6",
                                    "Decodes the ROM as ARM6 (MYK82)."
                                    );
    parser.addOption(arm6Option);
    QCommandLineOption msp430Option(QStringList()<<"decode-msp430",
                                    "Decodes the ROM as MSP430. (Broken.)"
                                    );
    parser.addOption(msp430Option);
    QCommandLineOption tlcsfontOption(QStringList()<<"decode-tlcs47font",
                                  "Decodes as a TMP47C434N Font."
                                  );
    parser.addOption(tlcsfontOption);
    QCommandLineOption colsdownlswapOption(QStringList()<<"decode-cols-downl-swap",
                                      "Decodes as a uCOM4 ROM."
                                      );
    parser.addOption(colsdownlswapOption);

    //Decoders for Zorrom compatibility.
    QCommandLineOption colsdownrOption(QStringList()<<"decode-cols-downr",
                                       "Decodes the ROM first down then right like a Gameboy."
                                       );
    parser.addOption(colsdownrOption);
    QCommandLineOption colsdownlOption(QStringList()<<"decode-cols-downl",
                                       "Decodes the ROM first down then left."
                                       );
    parser.addOption(colsdownlOption);
    QCommandLineOption colsleftOption(QStringList()<<"decode-cols-left",
                                      "Decodes the ROM left-to-right."
                                      );
    parser.addOption(colsleftOption);
    QCommandLineOption colsrightOption(QStringList()<<"decode-cols-right",
                                      "Decodes the ROM right-to-left."
                                      );
    parser.addOption(colsrightOption);
    QCommandLineOption squeezelrOption(QStringList()<<"decode-squeeze-lr",
                                       "Decodes even bits from the left, odd bits from right like in the TMS32C15."
                                       );
    parser.addOption(squeezelrOption);
    QCommandLineOption zorromOption(QStringList()<<"z"<<"zorrom",
                                       "Zorrom compatibility mode, with flipx before rotation."
                                       );
    parser.addOption(zorromOption);


    //ASCII dumping.
    QCommandLineOption asciiOption(QStringList()<<"a"<<"print-bits",
                                    "Prints ASCII art of the transformed bits."
                                    );
    parser.addOption(asciiOption);
    QCommandLineOption ASCIIOption(QStringList()<<"A"<<"print-pretty-bits",
                                    "Prints ASCII art with spaces."
                                    );
    parser.addOption(ASCIIOption);


    //Solver and graders.
    QCommandLineOption solveOption(QStringList()<<"solve",
                                   "Solves for an unknown format."
                                   );
    parser.addOption(solveOption);
    QCommandLineOption bytesOption(QStringList()<<"solve-bytes",
                                   "Bytes as a hint to the solver. 0:31,1:fe,2:ff",
                                   "bytes",
                                   ""
                                   );
    parser.addOption(bytesOption);
    QCommandLineOption stringOption(QStringList()<<"solve-string",
                                   "Byte string as a hint to the solver. 31,fe,ff",
                                   "bytes",
                                   ""
                                   );
    parser.addOption(stringOption);

    
    //Actually read the arguments.
    parser.process(a);

    QByteArray byteArray;
    GatoROM *gr=0;

    const QStringList args = parser.positionalArguments();

    if(args.count()==1 && parser.isSet(seanriddleOption)){  //Raw binary mode.
        QFile input(args[0]);
        input.open(QFile::ReadOnly);
        byteArray=input.readAll();
        bool okay=false;
        gr=new GatoROM(byteArray,parser.value(seanriddleOption).toInt(&okay));
        if(!okay){
            qDebug()<<"Not a valid row length:"<<parser.value(seanriddleOption);
            exit(1);
        }
    }else if(args.count()==1){                              //Main ASCII art mode.
        //Open the file.
        QFile input(args[0]);
        input.open(QFile::ReadOnly);
        byteArray=input.readAll();
        gr=new GatoROM(QString(byteArray));
    }else if(parser.isSet(randomOption)){                   //Fake a random ROM.
        //Initialize a random generator.
        QRandomGenerator *rng=QRandomGenerator::global();
        int rows=16*(rng->generate()%16);
        int cols=16*(rng->generate()%16);

        //Make it a string.
        QString str="";
        str.reserve(rows*cols*2);  //A little more than the expected length.
        for(int i=0; i<rows; i++){
            for(int j=0; j<cols; j++){
                str+=((rng->generate()&1)?"1":"0");
            }
            str+="\n";
        }

        //Make it a GatROM.
        gr=new GatoROM(str);
    }else{
        //Kindly help when the arguments make no sense.
        parser.showHelp(1);
    }


    //However we've opened a rom, now we can operate on it.
    if(gr){
        //Compatibility.
        if(parser.isSet(zorromOption))
            gr->zorrommode=1;

        //Rotations.
        gr->rotate(parser.value(rotateOption).toInt(), true);

        //Flips.
        if(parser.isSet(flipxOption))
            gr->flipx(true);
        if(parser.isSet(flipyOption))
            gr->flipy(true);

        //Inversion.
        if(parser.isSet(invertOption))
            gr->invert(true);

        //Export results.
        if(parser.isSet(asciiOption))
            std::cout<<gr->exportString(false).toStdString();
        if(parser.isSet(ASCIIOption))
            std::cout<<gr->exportString(true).toStdString();

        //Parsing formats.
        if(parser.isSet(infoOption))
            gr->decoder=new GatoDecoderInfo();
        else if(parser.isSet(arm6Option))
            gr->decoder=new GatoDecoderARM6();
        else if(parser.isSet(msp430Option))
            gr->decoder=new GatoDecoderMSP430();
        else if(parser.isSet(tlcsfontOption))
            gr->decoder=new GatoDecoderTLCSFont();
        else if(parser.isSet(colsdownlswapOption))
            gr->decoder=new GatoDecoderColsDownLSwap();
        else if(parser.isSet(colsdownrOption))
            gr->decoder=new GatoDecoderColsDownR();
        else if(parser.isSet(colsdownlOption))
            gr->decoder=new GatoDecoderColsDownL();
        else if(parser.isSet(colsleftOption))
            gr->decoder=new GatoDecoderColsLeft();
        else if(parser.isSet(colsrightOption))
            gr->decoder=new GatoDecoderColsRight();
        else if(parser.isSet(squeezelrOption))
            gr->decoder=new GatoDecoderSqueezeLR();


        //Attempt to decode even if we aren't saving it.
        if(gr->decoder){
            QByteArray ba;
            ba=gr->decode();


            //We can only decode to a binary if a decoder and output file have been chosen.
            if(parser.value(outputOption)!=""){
                QFile outfile(parser.value(outputOption));
                outfile.open(QIODevice::WriteOnly);
                outfile.write(ba);
                outfile.close();
            }

            if(!gr->checkSanity()){
                qDebug()<<"Failed GatoROM sanity check, exiting.";
                return 1;  //Failure code.
            }
        }else if(parser.isSet(solveOption)){
            //We don't know the settings, so we'll try everything and grade them.
            GatoGrader *grader=0;

            QString bytes=parser.value(bytesOption);
            QString string=parser.value(stringOption);

            if(bytes.length()>0){
                grader=new GatoGraderBytes(bytes);
            }else if(string.length()>0){
                grader=new GatoGraderString(string);
            }else{
                qDebug()<<"No solver criteria has been specified.  Try --solve-bytes or --solve-string.";
                return 1;
            }

            GatoSolver solver(gr, grader);
            for(solver.init(); !solver.finished(); solver.next()){
                if(solver.grade()>0){
                    std::cout<<"Grade "<<solver.grade()<<"   \t"
                              <<gr->preview().toStdString()<<"\t"
                              <<gr->description().toStdString()<<"\n";
                }

                //Perfect solutions go to the output file.
                if(solver.grade()==100 && parser.value(outputOption)!=""){
                    std::cout<<"Exporting\t"<<gr->description().toStdString()<<"\n";
                    QFile outfile(parser.value(outputOption));
                    outfile.open(QIODevice::WriteOnly);
                    outfile.write(gr->decoded);
                    outfile.close();
                }
            }
        }

        if(parser.isSet(printOption)){
            QPrinter printer;

            QPrintDialog dialog(&printer);
            dialog.setWindowTitle("Print Bits");
            if (dialog.exec() == QDialog::Accepted)
                gr->print(printer);
        }else if(parser.isSet(printpdfOption)){
            QPrinter printer(QPrinter::HighResolution);
            printer.setOutputFileName(parser.value(printpdfOption));
            gr->print(printer);
        }

        //Done with the file.
        delete gr;
    }else{
        return 1; //Error when failed to open.
    }

    return 0;
}

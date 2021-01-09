#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <fstream>
#include <fstream>

#include "lexer.h"
#include "parser.h"
#include "generator.h"

using namespace std;

void print_usage(const char *progname) {

    if (optopt == 'c')
        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
    else if (isprint (optopt))
        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    else {
        cout << "Usage " << progname << " -c path to file";
        fprintf (stdout,
                 "Unknown option character `\\x%x'.\n",
                 optopt);
    }
}

int main(int argc, char* argv[]) {
    bool astflag = false;
    bool lexflag = false;
    bool dotflag = false;
    char *outfile = nullptr;
    int index;
    int c;

    opterr = 0;

    while ((c = getopt(argc, argv, "haldc:")) != -1)
        switch (c)
        {
        case 'a':
            astflag = 1;
            break;
        case 'l':
            lexflag = 1;
            break;
        case 'd':
            dotflag = true;
            break;
        case 'o':
            outfile = optarg;
            break;
        case '?': case 'h':
            print_usage(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            abort ();
        }

    //printf ("aflag = %d, bflag = %d, cvalue = %s\n",
    //        aflag, bflag, cvalue);

    string filename = argv[argc-1];
    string outname(filename.c_str(), filename.length() -2); // cut the '.c'
    if (outfile)
        outname = outfile;
    //for (index = optind; index < argc; index++)
    //    printf ("Non-option argument %s\n", argv[index]);

    ifstream infile(filename);
    if (infile.is_open()) {

        std::string str((std::istreambuf_iterator<char>(infile)),
                         std::istreambuf_iterator<char>());
        infile.close();

        Cmp::Lexer lex(true);
        const char *cstr = str.c_str();
        if (!lex.tokenize(&cstr, filename.c_str()))
            fprintf(stderr, "Failed to tokenize file: %s\n", filename.c_str());
//        else
//            fprintf(stdout, "Successfully tokenized file: %s\n", filename);

        if (lexflag) {
            string lexfn = filename;
            lexfn += ".lex";
            ofstream olex(lexfn);
            if (olex.is_open()) {
                olex << lex.to_string(filename.c_str());
            }
            olex.close();
        }

        // parse to a AST
        Cmp::Parser parser(&lex, filename.c_str());
        if (parser.isValid()) {
            //cout << "Succesfully parsed to a ast" << endl;
            if (astflag) {
                string astfn = filename;
                astfn += ".ast";
                ofstream oast(astfn);
                if (oast.is_open())
                    oast<< parser.to_string();
                oast.close();
            }

            if (dotflag) {
                string dotfn = filename; dotfn += ".dot";
                ofstream odot(dotfn);
                if (odot.is_open())
                    odot << parser.to_dot(parser.root());
                odot.close();
            }

            // generate asm code
            Cmp::Generator gen(&parser, &lex);
            string asmStr = gen.generate(parser.root());
            if (!asmStr.empty()) {
                string asmFileName(filename); asmFileName += ".S";
                ofstream asmFile(asmFileName);
                if (asmFile.is_open()) {
                    asmFile << asmStr;
                    asmFile.close();

                    // invoke gcc assembler
                    string gccCmd = "gcc -m32 -g " + asmFileName + " -o " + outname + "  >test.txt";
                    int status = std::system(gccCmd.c_str());
                    std::cout << std::ifstream("test.txt").rdbuf();
                    std::system("rm test.txt");
                    //std::cout << "Exit code: " << WEXITSTATUS(status) << std::endl;
                }
            } else {
                cerr << "Failed to generate assembler code\n";
                exit(1);
            }

        } else {
            cerr << "Failed to parse file:" << filename << endl;
            exit(1);
        }

    } else {
        fprintf(stderr, "Could not open filen: %s\n", filename.c_str());
        exit(1);
    }

    exit(0);
}

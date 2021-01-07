#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <fstream>

#include "lexer.h"
#include "parser.h"

using namespace std;

void print_usage(int c) {

    if (optopt == 'c')
        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
    else if (isprint (optopt))
        fprintf (stderr, "Unknown option `-%c'.\n", optopt);
    else
        fprintf (stderr,
                 "Unknown option character `\\x%x'.\n",
                 optopt);
}

int main(int argc, char* argv[]) {
    int aflag = 0;
    int bflag = 0;
    char *cvalue = nullptr;
    int index;
    int c;

    opterr = 0;

    while ((c = getopt(argc, argv, "habc:")) != -1)
        switch (c)
        {
        case 'a':
            aflag = 1;
            break;
        case 'b':
            bflag = 1;
            break;
        case 'c':
            cvalue = optarg;
            break;
        case '?': case 'h':
            print_usage(c);
            return 1;
        default:
            print_usage(c);
            abort ();
        }

    printf ("aflag = %d, bflag = %d, cvalue = %s\n",
            aflag, bflag, cvalue);

    const char *filename = argv[argc-1];
    for (index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);

    ifstream infile(filename);
    if (infile.is_open()) {

        std::string str((std::istreambuf_iterator<char>(infile)),
                         std::istreambuf_iterator<char>());
        infile.close();

        Cmp::Lexer lex(true);
        const char *cstr = str.c_str();
        if (lex.tokenize(&cstr, filename))
            fprintf(stdout, "Successfully tokenized file: %s\n", filename);
        else
            fprintf(stderr, "Failed to tokenize file: %s\n", filename);

        string lexfn = filename;
        lexfn += ".lex";
        ofstream olex(lexfn);
        if (olex.is_open()) {
            olex << lex.to_string(filename);
        }
        olex.close();

        Cmp::Parser parser(&lex, filename);
        if (parser.isValid()) {
            cout << "Succesfully parsed to a ast" << endl;
            string astfn = filename;
            astfn += ".ast";
            ofstream oast(astfn);
            if (oast.is_open())
                oast<< parser.to_string();
            oast.close();

            string dotfn = filename; dotfn += ".dot";
            ofstream odot(dotfn);
            if (odot.is_open())
                odot << parser.to_dot(parser.root());
            odot.close();

        } else
            cerr << "Failed to parse file:" << filename << endl;


    } else {
        fprintf(stderr, "Could not open filen: %s\n", filename);
        return 1;
    }


    return 0;
}

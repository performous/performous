#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <iostream>

#define KEY_LEN 11

// Code originaly credited by "Invo" and provided ASIS
// found here: http://www.scorehero.com/forum/viewtopic.php?t=39923
// This code code be use to "decrypt" ".xen" files used in at least GH3 PC
// key should be guessed (xor)

char SwapByteBits(unsigned char cInput) {
    unsigned char nResult=0;
    
    for(unsigned int i = 0 ; i < 8 ; i++) {
        nResult = nResult << 1;
        nResult |= (cInput & 1);
        cInput = cInput >> 1;
    }
    return nResult;
}


// TODO: use boost iterator to decode in one line
int main(int argc, char** argv) {
    unsigned char cpKey[KEY_LEN] = "5atu6w4zaw";
    
    // Check params
    if (argc == 3) {
        // Open files
        std::ofstream outputFile(argv[2], std::ofstream::binary);
        std::ifstream inputFile(argv[1], std::ios::binary);
        
        // Get the file size
        inputFile.seekg (0, std::ios::end);
        unsigned int fileSize = inputFile.tellg();
        inputFile.seekg (0, std::ios::beg);
        
        // Allocate buffer
        char *pBuffer = new char[fileSize];
        
        // Reading inputfile
        std::cout << "Reading input file (" << fileSize << " Bytes)... ";
        inputFile.read(pBuffer, fileSize);
        inputFile.close();
        std::cout << "DONE!" << std::endl;
        
        // Decrypt
        std::cout << "Decrypting... ";
        char *pInput = pBuffer;
        for (unsigned int nLoopIdx = 0 ; nLoopIdx < fileSize ; nLoopIdx++) {
            *pInput = SwapByteBits(*pInput ^ cpKey[nLoopIdx % (KEY_LEN-1)]);
            pInput++;
        }
        std::cout << "DONE!" << std::endl;
        
        // Write the output
        std::cout << "Writing outpout file... ";
        outputFile.write(pBuffer, fileSize);
        outputFile.close();
        delete [] pBuffer;
        std::cout << "DONE!" << std::endl;
        
        return EXIT_SUCCESS;
    }
    else {
        std::cout << "Usage: " << argv[0] << " <Input> <Output>" << std::endl;
        return EXIT_FAILURE;
    }
}

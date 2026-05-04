
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>

#include <unistd.h>
#include <pthread.h>

#include "LineInfo.h"

using namespace std;

pthread_mutex_t lock;
static unsigned totalLineCount;

void GetLyricLinesFromFileToVector(string          lyricsFilenameStr,
                                   vector<string>& lyricLinesVector,
                                   unsigned&       noOfLyricLines) {

//$$
    ifstream fileLyricsStream(lyricsFilenameStr);
    if(fileLyricsStream.fail())
        throw domain_error(LineInfo("File open failure ", __FILE__, __LINE__));

        string line;
        while(getline(fileLyricsStream, line))
            lyricLinesVector.push_back(line);

            noOfLyricLines = lyricLinesVector.size();

}

struct SingLinesThreadInfoStruct {
    unsigned*      singerNoIdPtr;
    vector<string> lyricLinesVector;
    unsigned       noOfLyricLines;
};

// cout is not thread safe, so all couts need to be locked and then unlocked
void* SingLinesThread(void* singLinesThreadInfoStructPtr) {

    SingLinesThreadInfoStruct* threadInfoPtr = (SingLinesThreadInfoStruct*)singLinesThreadInfoStructPtr;
    static unsigned previousNoId;

    const unsigned maxSingLineNo    = 10;
    unsigned*      singersNoIdPtr   = threadInfoPtr->singerNoIdPtr;
    vector<string> lyricLinesVector = threadInfoPtr->lyricLinesVector;
    unsigned       noOfLyricLines   = threadInfoPtr->noOfLyricLines;

    // $$ lock the mutex
    if((pthread_mutex_lock(&lock)) != 0)
        throw domain_error(LineInfo("pthread_mutex_lock ", __FILE__, __LINE__));

    //$$ display start thread message
        cout << endl << "start  thread " << *singersNoIdPtr << endl;

    // $$ unlock the mutex
    if((pthread_mutex_unlock(&lock)) != 0)
        throw domain_error(LineInfo("pthread_mutex_unlock ", __FILE__, __LINE__));

    //sing up to 10 lines at a time
    unsigned singLimit    = 0;

    for (unsigned lyricLineNo = 0; lyricLineNo < noOfLyricLines; lyricLineNo+= maxSingLineNo){

        unsigned currentLineSungNo = lyricLineNo;
        singLimit += maxSingLineNo;

        for ( ; (currentLineSungNo < singLimit) && (currentLineSungNo < noOfLyricLines); currentLineSungNo++) {

            // $$ lock the mutex
            if((pthread_mutex_lock(&lock)) != 0)
                throw domain_error(LineInfo("pthread_mutex_lock ", __FILE__, __LINE__));


            if (previousNoId != *singersNoIdPtr) // for dipslay readabilty
                cout << endl;

            //$$ display singer no and lyric line
            cout << setw(3) << *singersNoIdPtr << " : " << setw(3) << currentLineSungNo << " > " << lyricLinesVector[currentLineSungNo] << endl;

            //$$ update previousNoId and increment totalLineCount;
            previousNoId = *singersNoIdPtr;
            ++totalLineCount;

            // $$ unlock the mutex
            if((pthread_mutex_unlock(&lock)) != 0)
                throw domain_error(LineInfo("pthread_mutex_unlock ", __FILE__, __LINE__));


        }

        //$$ get up time slice for 1 second
        sleep(1);

    }

    // $$ lock the mutex
    if((pthread_mutex_lock(&lock)) != 0)
        throw domain_error(LineInfo("pthread_mutex_lock ", __FILE__, __LINE__));

    //$$ display done   thread with singersNoIdPtr
    cout << "done   thread " << *singersNoIdPtr << endl;

    // $$ lock the mutex
    if((pthread_mutex_unlock(&lock)) != 0)
        throw domain_error(LineInfo("pthread_mutex_unlock ", __FILE__, __LINE__));

    //$$ exit the pthread
    pthread_exit(NULL);

}


int main(int argc, char* argv[]) {

  try{

    if (argc !=3) {
        string errorStr = "Usage : ";
                errorStr = errorStr.append("\n./singers <lyrics filename.txt> <number of singers>");
                errorStr = errorStr.append("\nExample : \n./singers Poplife.txt 4/n ");
        throw domain_error(LineInfo(errorStr, __FILE__, __LINE__));
    }

    string         lyricsFilenameStr(argv[1]);
    unsigned       noOfSingers    = stoi(argv[2]);
    vector<string> lyricLinesVector;
    unsigned       noOfLyricLines = 0;

    GetLyricLinesFromFileToVector(lyricsFilenameStr, lyricLinesVector, noOfLyricLines);

    cout << endl << endl;
    cout << "Song Lyrics File Name is : " << lyricsFilenameStr << endl;
    cout << "Number of lyric lines is : " << noOfLyricLines << endl;
    cout << endl << endl;

    pthread_t*                 singersThreadIdPtr;
    unsigned*                  singersNoIdPtr;
    SingLinesThreadInfoStruct* singLinesThreadInfoStructPtr;

    singersThreadIdPtr           = new pthread_t[noOfSingers];
    singersNoIdPtr               = new unsigned [noOfSingers];
    singLinesThreadInfoStructPtr = new SingLinesThreadInfoStruct[noOfSingers];

    //$$ initialize the pthread mutex lock
    if((pthread_mutex_init(&lock, NULL)) != 0)
        domain_error(LineInfo("pthread_mutex_init ", __FILE__, __LINE__));

    // Create independent threads each of which will execute the pthread function
    for (unsigned singerNo = 0; singerNo < noOfSingers; ++singerNo) {

        unsigned *singerNoIdPtr = new unsigned;
        *singerNoIdPtr = singerNo+1;

        //$$ Fill the singLinesThreadInfoStructPtr[singerNo] with the corresponding variable values
        singLinesThreadInfoStructPtr[singerNo].singerNoIdPtr = singerNoIdPtr;
        singLinesThreadInfoStructPtr[singerNo].lyricLinesVector = lyricLinesVector;
        singLinesThreadInfoStructPtr[singerNo].noOfLyricLines = noOfLyricLines;

        // $$ lock the mutex

        if((pthread_mutex_lock(&lock)) != 0)
            throw domain_error(LineInfo("pthread_mutex_lock ", __FILE__, __LINE__));

        //$$ display create   thread with singersNoIdPtr
        cout << "create   thread " << *singerNoIdPtr << endl;

        // $$ lock the mutex
        if((pthread_mutex_unlock(&lock)) != 0)
            throw domain_error(LineInfo("pthread_mutex_unlock ", __FILE__, __LINE__));

        //$$ create the thread using function SingLinesThread
        int threadCreateReturn = pthread_create(&(singersThreadIdPtr[singerNo]), NULL, SingLinesThread, (void*)&singLinesThreadInfoStructPtr[singerNo]);

        if(threadCreateReturn != 0)
            throw domain_error(LineInfo("pthread_create", __FILE__, __LINE__));

    }//for

    // Wait until all threads have completed before main continues.
    // If main doesn't wait, main may continue to an exit which will terminate
    // the main process. When main terminates, all associated threads will also
    // terminate before the threads have completed.
    for(int singerNo = 0; singerNo < noOfSingers; ++singerNo)
        pthread_join(singersThreadIdPtr[singerNo], NULL);
    //$$ join to all the running threads

    cout << endl;
    cout << "Program done " << endl;
    cout << "Total Line count sung : " << totalLineCount << endl;
    cout << endl;

    //$$ pthread_mutex_destroy
    if((pthread_mutex_destroy(&lock)) != 0)
        throw domain_error(LineInfo("pthread_mutex_destroy ", __FILE__, __LINE__));
  }
  catch (exception& e) {
      cout << e.what() << endl;
      exit(EXIT_FAILURE);
  }//catch

  exit(EXIT_SUCCESS);
}
~

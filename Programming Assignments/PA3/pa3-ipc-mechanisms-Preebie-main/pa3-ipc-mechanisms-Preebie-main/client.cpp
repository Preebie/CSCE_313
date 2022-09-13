#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
#include <chrono>

using namespace std::chrono;

int main(int argc, char *argv[])
{
    auto start = high_resolution_clock::now();

    int c;
    int buffercap = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    string ipcmethods = "f";
    int numChannels = 1;
    vector<RequestChannel *> channels;

    while ((c = getopt(argc, argv, "p:t:e:m:f:c:i:")) != -1)
    {
        switch (c)
        {
        case 'p':
            p = atoi(optarg);
            break;
        case 't':
            t = atof(optarg);
            break;
        case 'e':
            ecg = atoi(optarg);
            break;
        case 'm':
            buffercap = atoi(optarg);
            break;
        case 'c':
            isnewchan = true;
            numChannels = atoi(optarg);
            break;
        case 'f':
            isfiletransfer = true;
            filename = optarg;
            break;
        case 'i':
            ipcmethods = optarg;
            break;
        }
    }

    if (fork() == 0)
    {

        char *args[] = {"./server", "-m", (char *)to_string(buffercap).c_str(), "-i", (char *)ipcmethods.c_str(), NULL};
        if (execvp(args[0], args) < 0)
        {
            perror("exec filed");
            exit(0);
        }
    }

    RequestChannel *control_chan = NULL;
    if (ipcmethods == "f")
    {
        control_chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
    }
    else if (ipcmethods == "q")
    {
        control_chan = new MQRequestChannel("control", RequestChannel::CLIENT_SIDE);
    }
    else if (ipcmethods == "s")
    {
        control_chan = new SHMRequestChannel("control", RequestChannel::CLIENT_SIDE, buffercap);
    }
    RequestChannel *chan = control_chan;
    if (isnewchan)
    {
        if (ipcmethods == "f")
        {
            for (int i = 0; i < numChannels; i++)
            {
                cout << "Using the new channel everything following" << endl;
                MESSAGE_TYPE m = NEWCHANNEL_MSG;
                control_chan->cwrite(&m, sizeof(m));
                char newchanname[100];
                control_chan->cread(newchanname, sizeof(newchanname));
                chan = new FIFORequestChannel(newchanname, RequestChannel::CLIENT_SIDE);
                channels.push_back(chan);
                cout << "New channel by the name " << newchanname << " is created" << endl;
            }
        }
        else if (ipcmethods == "q")
        {
            for (int i = 0; i < numChannels; i++)
            {
                MESSAGE_TYPE m = NEWCHANNEL_MSG;
                control_chan->cwrite(&m, sizeof(m));
                char newchanname[100];
                control_chan->cread(newchanname, sizeof(newchanname));
                chan = new MQRequestChannel(newchanname, RequestChannel::CLIENT_SIDE);
                channels.push_back(chan);
                cout << "New channel by the name " << newchanname << " is created" << endl;
            }
        }
        else if (ipcmethods == "s")
        {
            for (int i = 0; i < numChannels; i++)
            {
                MESSAGE_TYPE m = NEWCHANNEL_MSG;
                control_chan->cwrite(&m, sizeof(m));
                char newchanname[100];
                control_chan->cread(newchanname, sizeof(newchanname));
                chan = new SHMRequestChannel(newchanname, RequestChannel::CLIENT_SIDE, buffercap);
                channels.push_back(chan);
                cout << "New channel by the name " << newchanname << " is created" << endl;
            }
        }
        cout << "All further communication will happen through it instead of the main channel" << endl;
    }

    if (!isfiletransfer)
    {
        ofstream myFile;
        myFile.open("received/x1.csv");
        if (numChannels > 1)
        {
            for (int k = 0; k < numChannels; k++)
            {
                if (t >= 0)
                {
                    datamsg d(p, t, ecg);
                    channels[k]->cwrite(&d, sizeof(d));
                    double ecgvalue;
                    channels[k]->cread(&ecgvalue, sizeof(double));
                    cout << "Ecg " << ecg << " value for patient " << p << " at time " << t << " is: " << ecgvalue << endl;
                }
                else
                {
                    double newTime = 0.0;
                    for (int i = 0; i < 1000; i++)
                    {
                        myFile << newTime << flush;
                        for (int j = 1; j < 3; j++)
                        {
                            ecg = j;
                            datamsg d(p, newTime, ecg);
                            channels[k]->cwrite(&d, sizeof(datamsg));

                            double result;
                            channels[k]->cread(&result, sizeof(double));

                            myFile << "," << result << flush;
                        }
                        newTime += 0.004;
                        myFile << endl;
                    }
                }
            }
            myFile.close();
        }
        else
        {
            if (t >= 0)
            {
                datamsg d(p, t, ecg);
                chan->cwrite(&d, sizeof(d));
                double ecgvalue;
                chan->cread(&ecgvalue, sizeof(double));
                cout << "Ecg " << ecg << " value for patient " << p << " at time " << t << " is: " << ecgvalue << endl;
            }
            else
            {
                ofstream myfile;
                myfile.open(("received/x1.csv"));

                t = 0.0;
                for (int i = 0; i < 1000; i++)
                {
                    myfile << t << flush;
                    for (int j = 1; j < 3; j++)
                    {
                        ecg = j;
                        datamsg d(p, t, ecg);
                        chan->cwrite(&d, sizeof(datamsg));

                        double result;
                        chan->cread(&result, sizeof(double));

                        myfile << "," << result << flush;
                    }
                    t += 0.004;
                    myfile << endl;
                }
                myfile.close();
            }
        }
    }
    else if (isfiletransfer)
    {
        if (numChannels > 1)
        {
            auto start = high_resolution_clock::now();
            filemsg fm(0, 0);
            string fname = filename;
            int len = sizeof(filemsg) + fname.size() + 1;
            char buf2[len];
            memcpy(buf2, &fm, sizeof(filemsg));
            strcpy(buf2 + sizeof(filemsg), fname.c_str());
            control_chan->cwrite(buf2, len);
            __int64_t fileLength;
            control_chan->cread(&fileLength, sizeof(fileLength));
            FILE *wFile = fopen(("received/" + fname).c_str(), "wb");
            char buf[buffercap];
            fm.length = buffercap;
            for (int j = 0; j < floor(fileLength / buffercap); j++)
            {
                memcpy(buf2, &fm, sizeof(filemsg));
                strcpy(buf2 + sizeof(filemsg), fname.c_str());
                channels[j % numChannels]->cwrite(buf2, len);
                channels[j % numChannels]->cread(buf, buffercap);
                fwrite(buf, sizeof(char), fm.length / sizeof(char), wFile);
                fm.offset += buffercap;

            }
            int val = fileLength % buffercap;
            if (val > 0)
            {
                fm.length = val;
                memcpy(buf2, &fm, sizeof(filemsg));
                strcpy(buf2 + sizeof(filemsg), fname.c_str());
                buf[val];
                channels[0]->cwrite(buf2, len);
                channels[0]->cread(buf, val);
                fwrite(buf, sizeof(char), val / sizeof(char), wFile);
                fclose(wFile);
            }
            auto stop = high_resolution_clock::now();
            chrono::duration<double> duration = (stop - start);
            cout << "File transfer " << duration.count() << endl;
        }
        else
        {
            filemsg fm(0, 0);
            string fname = filename;
            int len = sizeof(filemsg) + fname.size() + 1;
            char buf2[len];
            memcpy(buf2, &fm, sizeof(filemsg));
            strcpy(buf2 + sizeof(filemsg), fname.c_str());
            control_chan->cwrite(buf2, len);
            __int64_t fileLength;
            control_chan->cread(&fileLength, sizeof(fileLength));
            FILE *wFile = fopen(("received/" + fname).c_str(), "wb");
            char buf[buffercap];
            fm.length = buffercap;
            for (int i = 0; i < floor(fileLength / buffercap); i++)
            {
                memcpy(buf2, &fm, sizeof(filemsg));
                strcpy(buf2 + sizeof(filemsg), fname.c_str());
                chan->cwrite(buf2, len);
                chan->cread(buf, buffercap);
                fwrite(buf, sizeof(char), fm.length / sizeof(char), wFile);
                fm.offset += buffercap;
            }
            int val = fileLength % buffercap;
            if (val > 0)
            {
                fm.length = val;
                memcpy(buf2, &fm, sizeof(filemsg));
                strcpy(buf2 + sizeof(filemsg), fname.c_str());
                buf[val];
                chan->cwrite(buf2, len);
                chan->cread(buf, val);
                fwrite(buf, sizeof(char), val / sizeof(char), wFile);
                fclose(wFile);
            }
            
        }
    }
        auto stop = high_resolution_clock::now();
        chrono::duration<double> duration = (stop - start);
        cout << "File transfer " << duration.count() << endl;
    MESSAGE_TYPE q = QUIT_MSG;

    control_chan->cwrite(&q, sizeof(MESSAGE_TYPE));
    for (int i = 0; i < channels.size(); i++)
    {
        channels[i]->cwrite(&q, sizeof(MESSAGE_TYPE));
        delete channels.at(i);
        
    }
        
    wait(0);
}
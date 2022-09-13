#include <utility>
#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "TCPreqchannel.h"
using namespace std;

struct holding
{
    int patient;
    double reading;
};

void file_function(BoundedBuffer *requestbuffer, TCPRequestChannel *chan, string file, int bytes, string filestring)
{
    filemsg length_message(0, 0);
    int fullsize = sizeof(filemsg) + filestring.size() + 1;
    char *buffer = new char[fullsize];
    memcpy(buffer, &length_message, sizeof(filemsg));
    strcpy(buffer + sizeof(filemsg), filestring.c_str());
    chan->cwrite(buffer, fullsize);
    __int64_t number = 0;

    chan->cread(&number, sizeof(number));
    cout << number << endl;
    int message_count = ceil(double(number) / bytes);
    filemsg window_size(0, 0);

    if (message_count == 1)
    {
        window_size.offset = 0;
        window_size.length = number;
    }
    else
    {
        window_size.length = bytes;
        window_size.offset = 0;
    }
    long int message_number = number % bytes;
    char *response[bytes];

    for (int i = 0; i < message_count; i++)
    {

        if (i == message_count - 1)
        {

            filemsg m1(window_size.offset, message_number);
            requestbuffer->push((char *)&m1, sizeof(filemsg));
        }
        else
        {
            filemsg m2(window_size.offset, bytes);
            requestbuffer->push((char *)&m2, sizeof(filemsg));
            window_size.offset += bytes;
            
        }
    }
}

void patient_thread_function(int n, int pointnum, BoundedBuffer *buffer1)
{
    int count = 0;
    double num = 0.0;
    while (count != pointnum)
    {
        datamsg x(n, num, 1);
        datamsg y(n, num, 2);

        buffer1->push((char *)&x, sizeof(datamsg));
        buffer1->push((char *)&y, sizeof(datamsg));

        count++;
        num += .004;
    }
}

void worker_thread_function(TCPRequestChannel *chan, BoundedBuffer *request_buf, BoundedBuffer *response_buf, mutex *muter, FILE *fp, string filename)
{
    while (true)
    {
        char arr[1024];

        request_buf->pop(arr, 1024);
        MESSAGE_TYPE quitter = QUIT_MSG;
        if (*((MESSAGE_TYPE *)arr) == QUIT_MSG)
        {
            chan->cwrite(&quitter, sizeof(MESSAGE_TYPE));
            break;
        }
        datamsg *pointarr = (datamsg *)arr;
        filemsg *filemsgf = (filemsg *)arr;
        MESSAGE_TYPE mess = pointarr->mtype;
        MESSAGE_TYPE mess2 = filemsgf->mtype;
        int patient_number = pointarr->person;
        int ekg_number = pointarr->ecgno;
        double time = pointarr->seconds;
        int len = filemsgf->length;
        int off = filemsgf->offset;

        if (mess == DATA_MSG)
        {
            datamsg x(patient_number, time, ekg_number);
            chan->cwrite(&x, sizeof(datamsg));
            double reply;
            int nbytes = chan->cread(&reply, sizeof(double));
            holding h;
            h.patient = patient_number;
            h.reading = reply;
            response_buf->push((char *)(&h), sizeof(holding));
        }
        if (mess2 == FILE_MSG)
        {

            filemsg fm(off, len);
            memset(&fm, 0, sizeof(filemsg));
            fm.length = len;
            fm.offset = off;
            fm.mtype = FILE_MSG;
            char buf2[sizeof(filemsg) + filename.size()+1];
            char buf[len];

            memcpy(buf2, &fm, sizeof(filemsg));
            strcpy(buf2 + sizeof(filemsg), filename.c_str());
            chan->cwrite(buf2, sizeof(filemsg) + filename.size()+1);
            chan->cread(buf, len);

            unique_lock<mutex> l(*muter);
            fseek(fp, off, SEEK_SET);
            fwrite(buf, sizeof(char), len, fp);
            l.unlock();
        }
    }
    delete chan;
    return;
}
void histogram_thread_function(BoundedBuffer *response_buf, vector<Histogram *> Histvect)
{
    while (true)
    {
        char arr[1024];
        response_buf->pop(arr, 1024);
        holding *held = (holding *)arr;
        if (held->patient < 0)
        {
            break;
        }
        Histvect.at(held->patient - 1)->update(held->reading);
    }
}

int main(int argc, char *argv[])
{
    int n = 100;
    int p = 10;
    int w = 100;
    int b = 20;
    int m = MAX_MESSAGE;
    srand(time_t(NULL));
    int c;
    string file = "";
    bool file_transfer = false;
    int h = 10;
    string address = "";
    string portnum = "";
    while ((c = getopt(argc, argv, "n:p:w:b:m:f:h:a:r:")) != -1)
    {
        switch (c)
        {
        case 'n':
            n = atoi(optarg);
            break;
        case 'p':
            p = atoi(optarg);
            break;
        case 'w':
            w = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 'm':
            m = atoi(optarg);
            break;
        case 'f':
            file = optarg;
            file_transfer = true;
            break;
        case 'h':
            h = atoi(optarg);
            break;
        case 'a':
            address =optarg;
            break;
        case 'r':
            portnum = optarg;
            break;
        }
    }

    if (file_transfer)
    {

        TCPRequestChannel *chan = new TCPRequestChannel(address, portnum);
        BoundedBuffer requestbuffer(b);
        BoundedBuffer response_buffer(b);
        thread arrayw[w];

        TCPRequestChannel *chanw[w];
        struct timeval start, end;
        gettimeofday(&start, 0);
        for (int i = 0; i < w; i++)
        {
            TCPRequestChannel *channel = new TCPRequestChannel(address, portnum);
            chanw[i] = channel;
        }

        string path = string("./received/") + string(file);
        FILE *true_file = fopen(path.c_str(), "wb+");
        mutex muter;

        thread ff(file_function, &requestbuffer, chan, file, m, file);

        for (int i = 0; i < w; i++)
        {
            arrayw[i] = thread(worker_thread_function, chanw[i], &requestbuffer, &response_buffer, &muter, true_file, file);
        }
        ff.join();
        for (int i = 0; i < w; i++)
        {
            MESSAGE_TYPE quitter = QUIT_MSG;
            requestbuffer.push((char *)&quitter, sizeof(quitter));
        }
        for (int i = 0; i < w; i++)
        {
            arrayw[i].join();
        }
        fclose(true_file);
        gettimeofday(&end, 0);

        int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) / (int)1e6;
        int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) % ((int)1e6);
        cout << "Took " << secs + usecs * .000001 << " seconds" << endl;

        MESSAGE_TYPE q = QUIT_MSG;
        chan->cwrite((char *)&q, sizeof(MESSAGE_TYPE));
        cout << "I have finished like a cool kid" << endl;
        delete chan;
    }
    else
    {

        TCPRequestChannel *chan = new TCPRequestChannel(address, portnum);
        BoundedBuffer requestbuffer(b);
        BoundedBuffer response_buffer(b);
        HistogramCollection hc;
        thread arrayp[p];
        thread arrayw[w];
        thread arrayh[h];
        vector<Histogram *> Histers;
        for (int i = 0; i < p; i++)
        {
            Histogram *histogram = new Histogram(15, -10, 10);
            Histers.push_back(histogram);
        }
        TCPRequestChannel *chanw[w];
        struct timeval start, end;
        gettimeofday(&start, 0);
        for (int i = 0; i < w; i++)
        {
            TCPRequestChannel *channel = new TCPRequestChannel(address, portnum);
            chanw[i] = channel;
        }

        for (int i = 0; i < p; i++)
        {
            arrayp[i] = thread(patient_thread_function, i + 1, n, &requestbuffer);
        }
        for (int i = 0; i < w; i++)
        {
            arrayw[i] = thread(worker_thread_function, chanw[i], &requestbuffer, &response_buffer, nullptr, nullptr, "");
        }
        for (int i = 0; i < h; i++)
        {
            arrayh[i] = thread(histogram_thread_function, &response_buffer, Histers);
        }

        for (int i = 0; i < p; i++)
        {
            arrayp[i].join();
        }
        for (int i = 0; i < w; i++)
        {
            MESSAGE_TYPE quitter = QUIT_MSG;
            requestbuffer.push((char *)&quitter, sizeof(quitter));
        }
        for (int i = 0; i < w; i++)
        {
            arrayw[i].join();
        }
        for (int i = 0; i < h; i++)
        {
            holding holder;
            holder.patient = -1;
            holder.reading = 0.0;
            response_buffer.push((char *)&holder, sizeof(holding));
        }
        for (int i = 0; i < h; i++)
        {
            arrayh[i].join();
        }
        for (int i = 0; i < p; i++)
        {
            hc.add(Histers.at(i));
        }
        gettimeofday(&end, 0);

        hc.print();
        int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) / (int)1e6;
        int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec) % ((int)1e6);
        cout << "Took " << secs + usecs*.000001 << " seconds" << endl;

        MESSAGE_TYPE q = QUIT_MSG;
        chan->cwrite((char *)&q, sizeof(MESSAGE_TYPE));
        cout << "I have finished like a cool kid" << endl;
        delete chan;
    }
}

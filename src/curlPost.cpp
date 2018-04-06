/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Issue an HTTP POST and provide the data through the read callback.
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <curl/curl.h>

using std::string;

/* silly test data to POST */
static const char data[]="Lorem ipsum dolor sit amet, consectetur adipiscing "
  "elit. Sed vel urna neque. Ut quis leo metus. Quisque eleifend, ex at "
  "laoreet rhoncus, odio ipsum semper metus, at tempus ante urna in mauris. "
  "Suspendisse ornare tempor venenatis. Ut dui neque, pellentesque a varius "
  "eget, mattis vitae ligula. Fusce ut pharetra est. Ut ullamcorper mi ac "
  "sollicitudin semper. Praesent sit amet tellus varius, posuere nulla non, "
  "rhoncus ipsum.";

struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};

static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
    printf("## %s\n", __FUNCTION__);
    struct WriteThis *wt = (struct WriteThis *)userp;
    size_t buffer_size = size*nmemb;

    if(wt->sizeleft) {
        /* copy as much as possible from the source to the destination */
        size_t copy_this_much = wt->sizeleft;
        if(copy_this_much > buffer_size)
            copy_this_much = buffer_size;
        memcpy(dest, wt->readptr, copy_this_much);

        wt->readptr += copy_this_much;
        wt->sizeleft -= copy_this_much;
        return copy_this_much; /* we copied this many bytes */
    }

    return 0; /* no more data left to deliver */
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    printf("## %s\n", __FUNCTION__);
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc((char*)mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    printf("#%s\n", (char*)contents);

    return realsize;
}

int postJson2Server(string jsonStr, string& output){

    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* Check for errors */
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(res));
        return 1;
    }

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {

        /* First set the URL that is about to receive our POST. */
        //curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/index.cgi");
        //curl_easy_setopt(curl, CURLOPT_URL, "localhost");
        curl_easy_setopt(curl, CURLOPT_URL, "183.134.68.195/visualization/v1.0/data/organization/store");

        //curl_easy_setopt(curl, CURLOPT_PORT, 8090L);
        curl_easy_setopt(curl, CURLOPT_PORT, 8101L);


        char agent[1024] = { 0, };
        snprintf(agent, sizeof agent, "libcurl/%s", curl_version_info(CURLVERSION_NOW)->version);
        agent[sizeof agent - 1] = 0;
        curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

        /* Now specify we want to POST data */
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        /* get verbose debug output please */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);


#if 1
        //char json[1024];
        //sprintf(json, "{\"name\" : \"%s\", \"age\" : %d}", "asdfasd", 3344434);

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        //curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

#else //use read callback
        struct WriteThis wt;

        wt.readptr = data;
        wt.sizeleft = strlen(data);

        /* we want to use our own read function */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        /* pointer to pass to our read function */
        curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

        /*
           If you use POST to a HTTP 1.1 server, you can send data without knowing
           the size before starting the POST if you use chunked encoding. You
           enable this by adding a header like "Transfer-Encoding: chunked" with
           CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
           specify the size in the request.
           */
#ifdef USE_CHUNKED
        {
            struct curl_slist *chunk = NULL;

            chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
               list again */
        }
#else
        /* Set the expected POST size. If you want to POST large amounts of data,
           consider CURLOPT_POSTFIELDSIZE_LARGE */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);
#endif

#ifdef DISABLE_EXPECT
        /*
           Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
           header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
NOTE: if you want chunked transfer too, you need to combine these two
since you can only set one list of headers with CURLOPT_HTTPHEADER. */

        /* A less good option would be to enforce HTTP 1.0, but that might also
           have other implications. */
        {
            struct curl_slist *chunk = NULL;

            chunk = curl_slist_append(chunk, "Expect:");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            /* use curl_slist_free_all() after the *perform() call to free this
               list again */
        }
#endif

#endif

        struct MemoryStruct chunk;
        chunk.memory = (char*)malloc(1);  /* will be grown as needed by realloc above */ 
        chunk.size = 0;    /* no data at this point */ 

        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);


        /* set timeout */
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20); //TODO
        /* enable location redirects */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        /* set maximum allowed redirects */
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);


        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        else
        {
            char *ct;
            /* ask for the content-type */ 
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if((CURLE_OK == res) && ct)
                printf("We received Content-type: %s\n", ct);

            /*
             * Now, our chunk.memory points to a memory block that is chunk.size
             * bytes big and contains the remote file.
             *
             * Do something nice with it!
             */
            // printf("%s\n",chunk.memory);
            output = string(chunk.memory);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
        free(chunk.memory);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();

    return 0;
}

#ifdef DEBUG
int main(void)
{
    string output;
    postJson2Server("{\"testkey\":\"val1\",\"testkey2\":\"val2\"}", output);
    printf("output: %s\n", output.c_str());
    return 0;
}
#endif


/* debug
   int _main(void)
   {
   CURLcode ret;
   CURL *hnd;
   struct curl_slist *slist1;
   std::string jsonstr = "{\"username\":\"bob\",\"password\":12345}";

   slist1 = NULL;
   slist1 = curl_slist_append(slist1, "Content-type: application/json");

   hnd = curl_easy_init();
   curl_easy_setopt(hnd, CURLOPT_URL, "localhost");
   curl_easy_setopt(hnd, CURLOPT_PORT, 8080L);
   curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
   curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, jsonstr.c_str());
   curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
   curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
   curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
   curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

   printf("## %d\n", __LINE__);
   ret = curl_easy_perform(hnd);

   printf("## %d\n", __LINE__);
   curl_easy_cleanup(hnd);
   printf("## %d\n", __LINE__);

   hnd = NULL;
   curl_slist_free_all(slist1);
   printf("## %d\n", __LINE__);
   slist1 = NULL;
   return 0;
   }
   */

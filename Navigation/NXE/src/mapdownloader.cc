//============================================================================
// Name        : mapdownloader.cpp
// Author      : 
// Version     :
// Copyright   :
// Description : Navit Map downloader
//============================================================================

#include <iostream>
#include "mapdownloader.h"
using namespace std;


MapDownloader::MapDownloader() {
	m_mapFilePath = "/home/jlr02";
	m_reportProgess = false;
}


MapDownloader::~MapDownloader() {

}

size_t MapDownloader::dataWrite(void *buffer, size_t size, size_t nmemb, void *stream) {

	struct MapFile *out=(struct MapFile *)stream;
	if(out && !out->stream) {
		/* open file for writing */
		out->stream=fopen(out->filename, "wb");
		if(!out->stream) {
			onDownloadError(CURLE_WRITE_ERROR);
			return -1; /* failure, can't open file to write */
		}
	}
	return fwrite(buffer, size, nmemb, out->stream);
}

std::string MapDownloader::getDownloadErrorStr(CURLcode err) {

	switch (err) {
	case CURLE_WRITE_ERROR:
		return WRITE_FILE_ERR_STR;

	case CURLE_OPERATION_TIMEDOUT:
		return TIMEOUT_ERR_STR;

	case CURLE_COULDNT_CONNECT:
		return CONNECTION_ERR_STR;

	default:
		return UNKNOWN_ERR_STR;
	}
	return UNKNOWN_ERR_STR;
}

void MapDownloader::onDownloadError(CURLcode err)
{
	nDebug() << "onDownloadError: Code = " << err << " , " <<  curl_easy_strerror(err);

	if (cbOnError)
		cbOnError(getDownloadErrorStr(err));
}

int MapDownloader::onDownloadProgress(void *clientp,   curl_off_t dltotal,   curl_off_t dlnow,   curl_off_t ultotal,   curl_off_t ulnow) {

	nDebug() << "Download Progress: Total = " << dltotal " , Downloaded = " <<  dlnow;

	if (cbOnProgress)
		cbOnProgress(dltotal, dlnow);

	return CURLE_OK;
}

const char* MapDownloader::createMapRequestString(std::string name) {

	return "http://maps5.navit-project.org/api/map/?bbox=20.9,52.2,21.0,52.3&timestamp=150320";
}

CbOnError MapDownloader::cbOnError = 0;
CbOnProgress MapDownloader::cbOnProgress = 0;

int MapDownloader::download(std::string name) {
	CURL *curl;
	CURLcode res;

	std::string mapFileName = m_mapFilePath + "/" + name + ".bin";

	struct MapFile mapfile={
			mapFileName.c_str(), /* name to store the file as if succesful */
			NULL
	};

	nDebug() << "file name: " <<  mapfile.filename;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();

	if (curl) {

		curl_easy_setopt(curl, CURLOPT_URL, createMapRequestString(name));

	    /* Define our callback to get called when there's data to be written */
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataWrite);
	    /* Set a pointer to our struct to pass to the callback */
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mapfile);

	    /* Switch on full protocol/debug output */
	    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

	    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, onDownloadProgress);
	    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, m_reportProgess ? 0 : 1);

	    res = curl_easy_perform(curl);

	    if (res != CURLE_OK)
	    	onDownloadError(res);

	    /* always cleanup */
	    curl_easy_cleanup(curl);
	  }

	  if(mapfile.stream)
		  fclose(mapfile.stream); /* close the local file */

	  curl_global_cleanup();

	  return 0;
}


//============================================================================
// Name        : mapdownloader.cpp
// Author      : 
// Version     :
// Copyright   :
// Description : Navit Map downloader
//============================================================================

#include <iostream>
#include "mapdownloader.h"
#include "mapdesc.h"
#include "log.h"

using namespace std;


MapDownloader::MapDownloader() {
	m_mapFilePath = "/home/jlr02";
	m_mapDescFilePath = "/home/jlr02/osm_maps.xml";
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

long MapDownloader::getEstimatedSize(const std::string& name) {
	MapDesc mdesc;
	MapData m;

	if (mdesc.getMapData(name, m_mapDescFilePath, m))
		return m.size;
	else
		return 0;
}

const char* MapDownloader::createMapRequestString(const std::string& name) {

	MapDesc mdesc;
	MapData m;

	std::string res = "http://maps5.navit-project.org/api/map/?bbox=";

	if (mdesc.getMapData(name, m_mapDescFilePath, m)) {
		res += m.lon1 + "," + m.lat1+ "," + m.lon2 + "," + m.lat2 + "&timestamp=150320";
	} else	return NULL;

	nDebug() << "createMapRequestString : " << res;

	return res.c_str();
}

CbOnError MapDownloader::cbOnError = 0;
CbOnProgress MapDownloader::cbOnProgress = 0;

bool MapDownloader::download(const std::string& name) {
	CURL *curl;
	CURLcode res;
	bool downloaded=false;

	std::string mapFileName = m_mapFilePath + "/" + name + ".bin";

	struct MapFile mapfile={
			mapFileName.c_str(), /* name to store the file as if succesful */
			NULL
	};

	nDebug() << "file name: " <<  mapfile.filename;

	const char* reqs = createMapRequestString(name);

	if (reqs == NULL) {
		cbOnError(MAP_ERR_STR);
		return downloaded;
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();


	if (curl) {

		curl_easy_setopt(curl, CURLOPT_URL, reqs);

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
	    else
	    	downloaded=true;

	    /* always cleanup */
	    curl_easy_cleanup(curl);
	  }

	  if(mapfile.stream)
		  fclose(mapfile.stream); /* close the local file */

	  curl_global_cleanup();

	  return downloaded;
}


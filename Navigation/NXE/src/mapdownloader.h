/*
 * MapDownloader.h
 *
 *  Created on: Mar 31, 2015
 *      Author:
 */

#ifndef MAPDOWNLOADER_H_
#define MAPDOWNLOADER_H_

#include <curl/curl.h>
#include <string>

#define WRITE_FILE_ERR_STR "ERROR_WRITE"
#define TIMEOUT_ERR_STR    "ERROR_TIMEOUT"
#define CONNECTION_ERR_STR "ERROR_CONNECTION"
#define MAP_ERR_STR 	   "ERROR_MAP"
#define UNKNOWN_ERR_STR    "ERROR_UNKNOWN"


struct MapFile {
  const char *filename;
  FILE *stream;
};


typedef void (*CbOnError)(const std::string&);
typedef void (*CbOnProgress)(long, long);


class MapDownloader {
public:
	MapDownloader();

	bool download(const std::string& name);
	long getEstimatedSize(const std::string& name);

	virtual ~MapDownloader();

	inline void enableReportProgress(bool flag) {
		m_reportProgess = flag;
	}

	inline void setMapFileDir(const std::string& dir) {
		m_mapFilePath = dir;
	}

	inline void setMapDescFilePath(const std::string& path) {
		m_mapDescFilePath = path;
	}

	inline void setCbError(CbOnError cb) {
		cbOnError = cb;
	}

	inline void setCbProgress(CbOnProgress cb) {
		cbOnProgress = cb;
	}

	static CbOnError cbOnError;
	static CbOnProgress cbOnProgress;

private:

	static size_t dataWrite(void *buffer, size_t size, size_t nmemb, void *stream);
	static void onDownloadError(CURLcode err);
	static int onDownloadProgress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);


	static std::string getDownloadErrorStr(CURLcode err);


	const char* createMapRequestString(const std::string& name);

	std::string m_mapFilePath;
	std::string m_mapDescFilePath;

	bool m_reportProgess;

};

#endif /* MAPDOWNLOADER_H_ */

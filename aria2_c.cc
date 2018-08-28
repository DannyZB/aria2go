#include "aria2_c.h"
#include "_cgo_export.h"
#include "aria2.h"
#include <iostream>
#include <sstream>
#include <string.h>

std::vector<std::string> splitByComma(std::string in) {
  std::string val;
  std::stringstream is(in);
  std::vector<std::string> out;

  while (getline(is, val, ',')) {
    out.push_back(val);
  }
  return out;
}

const char *toCStr(std::string in) {
  int len = in.length();
  char *val = new char[len + 1];
  memcpy(val, in.data(), len);
  val[len] = '\0';
  return val;
}

const char *getFileName(std::string dir, std::string path) {
  if (path.find(dir, 0) == std::string::npos) {
    return toCStr(path);
  }
  int index = dir.size();
  std::string name = path.substr(index + 1);
  return toCStr(name);
}

/**
 * Global aria2 session.
 */
aria2::Session *session;
/**
 * Global aria2go go pointer.
 */
uint64_t ariagoPointer;

/**
 * Download event callback for aria2.
 */
int downloadEventCallback(aria2::Session *session, aria2::DownloadEvent event,
                          const aria2::A2Gid gid, void *userData) {
  notifyEvent(ariagoPointer, gid, event);
  return 0;
}

/**
 * Initial aria2 library.
 */
int init(uint64_t pointer) {
  ariagoPointer = pointer;
  int ret = aria2::libraryInit();
  aria2::SessionConfig config;
  config.keepRunning = true;
  config.downloadEventCallback = downloadEventCallback;
  session = aria2::sessionNew(aria2::KeyVals(), config);
  return ret;
}

/**
 * Adds new HTTP(S)/FTP/BitTorrent Magnet URI. See `addUri` in aria2.
 *
 * @param uri uri to add
 */
uint64_t addUri(char *uri) {
  std::vector<std::string> uris = {uri};
  aria2::KeyVals options;
  aria2::A2Gid gid;
  int ret = aria2::addUri(session, &gid, uris, options);
  if (ret < 0) {
    return 0;
  }

  return gid;
}

/**
 * Parse torrent file for given file path.
 */
struct TorrentInfo *parseTorrent(char *fp) {
  aria2::KeyVals options;
  options.push_back(std::make_pair("dry-run", "true"));

  aria2::A2Gid gid;
  int ret = aria2::addTorrent(session, &gid, fp, options);
  if (ret < 0) {
    return nullptr;
  }

  aria2::DownloadHandle *dh = aria2::getDownloadHandle(session, gid);
  if (!dh) {
    return nullptr;
  }

  std::vector<aria2::FileData> files = dh->getFiles();
  struct TorrentInfo *ti = new TorrentInfo();
  int total = files.size();
  std::string dir = dh->getDir();
  ti->totalFile = total;
  ti->infoHash = toCStr(dh->getInfoHash());
  struct FileInfo *allFiles = new FileInfo[total];
  for (int i = 0; i < files.size(); i++) {
    aria2::FileData file = files[i];
    struct FileInfo *fi = new FileInfo();
    fi->index = file.index;
    fi->name = getFileName(dir, file.path);
    fi->length = file.length;
    fi->selected = file.selected;

    allFiles[i] = *fi;
  }
  ti->files = allFiles;
  aria2::deleteDownloadHandle(dh);
  aria2::removeDownload(session, gid);

  return ti;
}

/**
 * Add bit torrent file. See `addTorrent` in aria2.
 */
uint64_t addTorrent(char *fp, const char *coptions) {
  aria2::KeyVals options;
  aria2::A2Gid gid;

  std::vector<std::string> o = splitByComma(std::string(coptions));
  for (int i = 0; i < o.size(); i += 2) {
    std::string key = o[i];
    std::string val = o[i + 1];
    options.push_back(std::make_pair(key, val));
  }

  int ret = aria2::addTorrent(session, &gid, fp, options);
  if (ret < 0) {
    return 0;
  }

  return gid;
}

/**
 * Change aria2 options. See `changeOption` in aria2.
 */
bool changeOptions(uint64_t gid, const char *options) {
  if (options == nullptr) {
    return true;
  }

  std::vector<std::string> o = splitByComma(std::string(options));
  /* key and val should be pair */
  if (o.size() % 2 != 0) {
    return false;
  }

  aria2::KeyVals aria2Options;
  for (int i = 0; i < o.size(); i += 2) {
    std::string key = o[i];
    std::string val = o[i + 1];
    aria2Options.push_back(std::make_pair(key, val));
  }

  return aria2::changeOption(session, gid, aria2Options) == 0;
}

/**
 * Start to download. This will block current thread.
 */
void start() {
  for (;;) {
    int ret = aria2::run(session, aria2::RUN_DEFAULT);
    if (ret != 1) {
      break;
    }
  }
}

/**
 * Pause an active download with given gid. This will mark the download to
 * `DOWNLOAD_PAUSED`. See `resume`.
 */
bool pause(uint64_t gid) { return aria2::pauseDownload(session, gid) == 0; }

/**
 * Resume a paused download with given gid. See `pause`.
 */
bool resume(uint64_t gid) { return aria2::unpauseDownload(session, gid) == 0; }

/**
 * Remove a download in queue. This will stop seeding(for torrent) and downloading.
 */
bool removeDownload(uint64_t gid) { return aria2::removeDownload(session, gid) == 0; }

/**
 * Get download information for current download with given gid.
 */
struct DownloadInfo *getDownloadInfo(uint64_t gid) {
  aria2::DownloadHandle *dh = aria2::getDownloadHandle(session, gid);
  if (!dh) {
    return nullptr;
  }

  struct DownloadInfo *di = new DownloadInfo();
  di->status = dh->getStatus();
  di->totalLength = dh->getTotalLength();
  di->bytesCompleted = dh->getCompletedLength();
  di->uploadLength = dh->getUploadLength();
  di->downloadSpeed = dh->getDownloadSpeed();
  di->uploadSpeed = dh->getUploadSpeed();

  return di;
}

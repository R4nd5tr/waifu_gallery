#include "../service/database.h"
#include "worker.h"

DatabaseWorker::DatabaseWorker(const QString &dbFile, QObject *parent) {
    database = PicDatabase(dbFile);
}
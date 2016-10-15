//
//  SQLiter.h
//  MXLM
//
//  Created by Jon Wayne on 15/7/4.
//
//

#ifndef __MXLM__SQLiter__
#define __MXLM__SQLiter__

#include "cocos2d.h"
#include "../../../../external/sqlite3/include/sqlite3.h"

using namespace cocos2d;

class SQLiter : CCObject
{
private:
    SQLiter();
    ~SQLiter();
public:  
    static SQLiter *getIntence();
private:
    bool init();
public:
    bool exec(const char *sql);
    bool deleteDB();
public:
    bool selectFromDB(lua_State *L);
private:
    sqlite3 *_sqlite;
};

extern int open_SQLiter(lua_State *L);

#endif /* defined(__MXLM__SQLiter__) */

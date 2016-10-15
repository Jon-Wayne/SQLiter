//
//  SQLiter.cpp
//  MXLM
//
//  Created by Jon Wayne on 15/7/4.
//
//

#include "SQLiter.h"

extern "C" {
#include "lua.h"
}

#include <stdio.h>

const char SQLITER_DB_FILE_NAME[] = "sqliter_file.db";
const char SQLITER_TABLE_TO_CLASS[] = "cpp_obj";

SQLiter *g_sqliterIntence = NULL;

SQLiter::SQLiter():
_sqlite(NULL)
{}

SQLiter::~SQLiter()
{
    
}

SQLiter *SQLiter::getIntence()
{
    if (!g_sqliterIntence) {
        g_sqliterIntence = new SQLiter();
        if (g_sqliterIntence && g_sqliterIntence->init()) {
            g_sqliterIntence->retain();
        } else {
            delete g_sqliterIntence;
            g_sqliterIntence = NULL;
        }
    }
    
    return g_sqliterIntence;
}

bool SQLiter::init()
{
    std::string path = CCFileUtils::sharedFileUtils()->getWritablePath();
    path.append(SQLITER_DB_FILE_NAME);
    CCLOG("SQLiter init path : %s", path.c_str());
    int result = sqlite3_open(path.c_str(), &_sqlite);
    if (result != SQLITE_OK) {
        CCLOG("SQLiter sqlite3_open : %d", result);
        return false;
    }
    return true;
}

bool SQLiter::exec(const char *sql)
{
    CCLOG("SQLiter exec sql : %s", sql);
    int result = SQLITE_OK;
    char message[128] = {0};
    char *pMessage = message;
    result = sqlite3_exec(_sqlite, sql, NULL, NULL, &pMessage);
    if (result != SQLITE_OK) {
        CCLOG("SQLiter sqlite3_exec : %d", result);
        return false;
    }
    return true;
}

// ------------------------------ C++ ==> Lua ------------------------------
bool SQLiter::deleteDB()
{
    sqlite3_close(_sqlite);
    
    std::string path = CCFileUtils::sharedFileUtils()->getWritablePath();
    path.append(SQLITER_DB_FILE_NAME);
    remove(path.c_str());
    
    return init();
}

bool SQLiter::selectFromDB(lua_State *L)
{
    lua_pop(L, 1);
    
    const char *sql = lua_tostring(L, -1);
    CCLOG("SQLiter selectFromDB sql : %s", sql);
    
    char * message = NULL;
    char **queryResults;
    int row, col;
    
    int result = SQLITE_OK;
    result = sqlite3_get_table(_sqlite, sql, &queryResults, &row, &col, &message);
    if (result != SQLITE_OK) {
        CCLOG("SQLiter sqlite3_exec : %d", result);
        return false;
    }
    
    CCLOG("%d rows are returned", row);
    if (row == 0) {
        lua_pushnil(L);
        return true;
    }
    
    lua_newtable(L);
    
    // row 1 : var name.
    lua_pushnumber(L, 1);
    lua_newtable(L);
    for (int j=0; j<col; j++) {
        printf("%s \t", queryResults[j]);
        lua_pushnumber(L, j + 1);
        lua_pushstring(L, queryResults[j]);
        lua_rawset(L, -3);
    }
    printf("\n");
    lua_rawset(L, -3);
    
    for (int i=1; i<=row; i++) {
        lua_pushnumber(L, i + 1);
        lua_newtable(L);
        for (int j=0; j<col; j++) {
            printf("%s \t", queryResults[i*col + j]);
            lua_pushnumber(L, j + 1);
            lua_pushstring(L, queryResults[i*col + j]);
            lua_rawset(L, -3);
        }
        printf("\n");
        lua_rawset(L, -3);
    }
    printf("\n");
    
    // 3 ... resul table ( with many results )
    // 2 ... strin sql
    // 1 ... cself SQLiter
    
    return true;
}

// ------------------------------ C ==> C++ ------------------------------
int SQLiter_getIntence(lua_State *L)
{
    SQLiter *obj = SQLiter::getIntence();
    if (!obj) {
        lua_pushnil(L);
        return 1;
    }
    
    // store c++ instance
    lua_newtable(L);
    lua_pushstring(L, SQLITER_TABLE_TO_CLASS);
    lua_pushlightuserdata(L, obj);
    lua_rawset(L, -3);
    
    // check
    lua_getglobal(L, "SQLiter");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        lua_pushnil(L);
        return 1;
    }
    lua_pop(L, 1);
    
    // create meta table
    lua_newtable(L);
    lua_pushstring(L, "__index");
    lua_getglobal(L, "SQLiter");
    lua_rawset(L, -3);
    lua_setmetatable(L, -2);
    
    return 1;
}

int SQLiter_deleteDB(lua_State *L)
{
    if (!lua_istable(L, -1)) {
        CCLOG("SQLiter deleteDB error!");
        return 1;
    }
    
    lua_pushstring(L, SQLITER_TABLE_TO_CLASS);
    lua_rawget(L, -2);
    
    if (!lua_isuserdata(L, -1)) {
        lua_pop(L, 1);
        CCLOG("SQLiter deleteDB error!");
        return 1;
    }
    
    SQLiter *obj = (SQLiter *)lua_touserdata(L, -1);
    if (!obj) {
        lua_pop(L, 1);
        CCLOG("SQLiter deleteDB error!");
        return 1;
    }
    
    if (!obj->deleteDB()) {
        lua_pushboolean(L, false);
        CCLOG("SQLiter deleteDB fail!");
        return 1;
    }
    
    lua_pushboolean(L, true);
    CCLOG("SQLiter deleteDB success!");
    
    return 1;
}

int SQLiter_exec(lua_State *L)
{
    if (!lua_istable(L, -2)) {
        CCLOG("SQLiter deleteDB error!");
        return 1;
    }
    
    lua_pushstring(L, SQLITER_TABLE_TO_CLASS);
    lua_rawget(L, -3);
    
    if (!lua_isstring(L, -2) || !lua_isuserdata(L, -1)) {
        lua_pop(L, 1);
        CCLOG("SQLiter createTable error!");
        return 1;
    }
    
    SQLiter *obj = (SQLiter *)lua_touserdata(L, -1);
    const char *sql = lua_tostring(L, -2);
    if (!obj || !sql) {
        lua_pop(L, 1);
        CCLOG("SQLiter createTable error!");
        return 1;
    }
    
    if (!obj->exec(sql)) {
        lua_pop(L, 1);
        lua_pushboolean(L, false);
        CCLOG("SQLiter createTable fail!");
        return 1;
    }
    
    lua_pop(L, 1);
    lua_pushboolean(L, true);
    CCLOG("SQLiter createTable success!");
    
    return 1;
}

int SQLiter_selectFromDB(lua_State *L)
{
    if (!lua_istable(L, -2)) {
        CCLOG("SQLiter deleteDB error!");
        return 1;
    }
    
    lua_pushstring(L, SQLITER_TABLE_TO_CLASS);
    lua_rawget(L, -3);
    
    if (!lua_isstring(L, -2) || !lua_isuserdata(L, -1)) {
        lua_pop(L, 1);
        CCLOG("SQLiter selectFromDB error!");
        return 1;
    }
    
    SQLiter *obj = (SQLiter *)lua_touserdata(L, -1);
    const char *sql = lua_tostring(L, -2);
    if (!obj || !sql) {
        lua_pop(L, 1);
        CCLOG("SQLiter selectFromDB error!");
        return 1;
    }
    
    if (!obj->selectFromDB(L)) {
        lua_pop(L, 1);
        lua_pushnil(L);
        CCLOG("SQLiter selectFromDB fail!");
        return 1;
    }
    
    CCLOG("SQLiter selectFromDB success!");
    
    return 1;
}

// ------------------------------ Lua => C ------------------------------
int open_SQLiter(lua_State *L)
{
    // check if SQLiter object exists.
    lua_getglobal(L, "SQLiter");
    if (!lua_isnil(L, -1)) {
        lua_pop(L, 1);
        CCLOG("SQLiter have been in global state!");
        return 0;
    }
    
    lua_pop(L, 1);
    
    // create SQLiter object.
    lua_newtable(L);
    lua_setglobal(L, "SQLiter");
    
    // add functions for SQLiter object.
    lua_getglobal(L, "SQLiter");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        CCLOG("SQLiter error!");
        return 0;
    }
    
    lua_pushcfunction(L, SQLiter_getIntence);
    lua_setfield(L, -2, "getIntence");
    lua_pushcfunction(L, SQLiter_deleteDB);
    lua_setfield(L, -2, "deleteDB");
    lua_pushcfunction(L, SQLiter_exec);
    lua_setfield(L, -2, "createTable");
    lua_pushcfunction(L, SQLiter_exec);
    lua_setfield(L, -2, "insertToDB");
    lua_pushcfunction(L, SQLiter_exec);
    lua_setfield(L, -2, "deleteFromDB");
    lua_pushcfunction(L, SQLiter_exec);
    lua_setfield(L, -2, "updateToDB");
    lua_pushcfunction(L, SQLiter_selectFromDB);
    lua_setfield(L, -2, "selectFromDB");
    
    lua_pop(L, 1);
    
    return 0;
}
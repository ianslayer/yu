#ifndef MODULE_H
#define MODULE_H

#if defined MODULE_IMPL
namespace yu
{

}

struct YuCore
{
};

#else
struct YuCore;
#endif

#define MODULE_INIT(name) void name(YuCore* core)
typedef MODULE_INIT(module_init);

#define MODULE_UPDATE(name) void name()
typedef MODULE_UPDATE(module_update);

#endif

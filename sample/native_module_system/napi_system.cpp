#include <napi.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef OHOS_PLATFORM
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/statvfs.h>
#endif

namespace {

// System information functions
napi_value GetOSInfo(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  struct utsname unameData;
  if (uname(&unameData) == 0) {
    napi_value sysname, nodename, release, version, machine;
    napi_create_string_utf8(env, unameData.sysname, NAPI_AUTO_LENGTH, &sysname);
    napi_create_string_utf8(env, unameData.nodename, NAPI_AUTO_LENGTH, &nodename);
    napi_create_string_utf8(env, unameData.release, NAPI_AUTO_LENGTH, &release);
    napi_create_string_utf8(env, unameData.version, NAPI_AUTO_LENGTH, &version);
    napi_create_string_utf8(env, unameData.machine, NAPI_AUTO_LENGTH, &machine);

    napi_set_named_property(env, result, "sysname", sysname);
    napi_set_named_property(env, result, "nodename", nodename);
    napi_set_named_property(env, result, "release", release);
    napi_set_named_property(env, result, "version", version);
    napi_set_named_property(env, result, "machine", machine);
  }
#else
  napi_value sysname, nodename, release, version, machine;
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &sysname);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &nodename);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &release);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &version);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &machine);

  napi_set_named_property(env, result, "sysname", sysname);
  napi_set_named_property(env, result, "nodename", nodename);
  napi_set_named_property(env, result, "release", release);
  napi_set_named_property(env, result, "version", version);
  napi_set_named_property(env, result, "machine", machine);
#endif

  return result;
}

napi_value GetMemoryInfo(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  struct sysinfo sysInfo;
  if (sysinfo(&sysInfo) == 0) {
    napi_value totalRAM, freeRAM, totalSwap, freeSwap;
    napi_create_number(env, static_cast<double>(sysInfo.totalram) / (1024 * 1024), &totalRAM);
    napi_create_number(env, static_cast<double>(sysInfo.freeram) / (1024 * 1024), &freeRAM);
    napi_create_number(env, static_cast<double>(sysInfo.totalswap) / (1024 * 1024), &totalSwap);
    napi_create_number(env, static_cast<double>(sysInfo.freeswap) / (1024 * 1024), &freeSwap);

    napi_set_named_property(env, result, "totalRAM", totalRAM);
    napi_set_named_property(env, result, "freeRAM", freeRAM);
    napi_set_named_property(env, result, "totalSwap", totalSwap);
    napi_set_named_property(env, result, "freeSwap", freeSwap);
  }
#else
  napi_value totalRAM, freeRAM, totalSwap, freeSwap;
  napi_create_number(env, 0, &totalRAM);
  napi_create_number(env, 0, &freeRAM);
  napi_create_number(env, 0, &totalSwap);
  napi_create_number(env, 0, &freeSwap);

  napi_set_named_property(env, result, "totalRAM", totalRAM);
  napi_set_named_property(env, result, "freeRAM", freeRAM);
  napi_set_named_property(env, result, "totalSwap", totalSwap);
  napi_set_named_property(env, result, "freeSwap", freeSwap);
#endif

  return result;
}

napi_value GetDiskInfo(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  std::string path = "/";
  if (argc > 0) {
    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    path = str;
    delete[] str;
  }

  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  struct statvfs stat;
  if (statvfs(path.c_str(), &stat) == 0) {
    napi_value total, free, available, used;
    napi_create_number(env, static_cast<double>(stat.f_blocks * stat.f_frsize) / (1024 * 1024), &total);
    napi_create_number(env, static_cast<double>(stat.f_bfree * stat.f_frsize) / (1024 * 1024), &free);
    napi_create_number(env, static_cast<double>(stat.f_bavail * stat.f_frsize) / (1024 * 1024), &available);
    napi_create_number(env, static_cast<double>((stat.f_blocks - stat.f_bfree) * stat.f_frsize) / (1024 * 1024), &used);

    napi_set_named_property(env, result, "total", total);
    napi_set_named_property(env, result, "free", free);
    napi_set_named_property(env, result, "available", available);
    napi_set_named_property(env, result, "used", used);
  }
#else
  napi_value total, free, available, used;
  napi_create_number(env, 0, &total);
  napi_create_number(env, 0, &free);
  napi_create_number(env, 0, &available);
  napi_create_number(env, 0, &used);

  napi_set_named_property(env, result, "total", total);
  napi_set_named_property(env, result, "free", free);
  napi_set_named_property(env, result, "available", available);
  napi_set_named_property(env, result, "used", used);
#endif

  return result;
}

napi_value GetProcessInfo(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  napi_value pid, ppid, uid, gid;
  napi_create_number(env, getpid(), &pid);
  napi_create_number(env, getppid(), &ppid);
  napi_create_number(env, getuid(), &uid);
  napi_create_number(env, getgid(), &gid);

  napi_set_named_property(env, result, "pid", pid);
  napi_set_named_property(env, result, "ppid", ppid);
  napi_set_named_property(env, result, "uid", uid);
  napi_set_named_property(env, result, "gid", gid);

  // Get process name
  char procName[256];
  snprintf(procName, sizeof(procName), "/proc/%d/comm", getpid());
  FILE* fp = fopen(procName, "r");
  if (fp) {
    if (fgets(procName, sizeof(procName), fp)) {
      // Remove newline
      size_t len = strlen(procName);
      if (len > 0 && procName[len-1] == '\n') {
        procName[len-1] = '\0';
      }
      napi_value name;
      napi_create_string_utf8(env, procName, NAPI_AUTO_LENGTH, &name);
      napi_set_named_property(env, result, "name", name);
    }
    fclose(fp);
  }
#else
  napi_value pid, ppid, uid, gid, name;
  napi_create_number(env, 0, &pid);
  napi_create_number(env, 0, &ppid);
  napi_create_number(env, 0, &uid);
  napi_create_number(env, 0, &gid);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &name);

  napi_set_named_property(env, result, "pid", pid);
  napi_set_named_property(env, result, "ppid", ppid);
  napi_set_named_property(env, result, "uid", uid);
  napi_set_named_property(env, result, "gid", gid);
  napi_set_named_property(env, result, "name", name);
#endif

  return result;
}

napi_value GetEnvironmentVariable(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 1) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing environment variable name");
    return nullptr;
  }

  size_t length;
  char* key;
  napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
  key = new char[length + 1];
  napi_get_value_string_utf8(env, args[0], key, length + 1, nullptr);

  const char* value = getenv(key);
  delete[] key;

  napi_value result;
  if (value) {
    napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &result);
  } else {
    napi_get_undefined(env, &result);
  }

  return result;
}

napi_value SetEnvironmentVariable(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2];
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  if (argc < 2) {
    napi_throw_error(env, "ERR_INVALID_ARGS", "Missing environment variable name or value");
    return nullptr;
  }

  size_t keyLength, valueLength;
  char* key; char* value;
  napi_get_value_string_utf8(env, args[0], nullptr, 0, &keyLength);
  key = new char[keyLength + 1];
  napi_get_value_string_utf8(env, args[0], key, keyLength + 1, nullptr);

  napi_get_value_string_utf8(env, args[1], nullptr, 0, &valueLength);
  value = new char[valueLength + 1];
  napi_get_value_string_utf8(env, args[1], value, valueLength + 1, nullptr);

  int status = setenv(key, value, 1);
  delete[] key;
  delete[] value;

  napi_value result;
  napi_create_boolean(env, status == 0, &result);
  return result;
}

napi_value GetUptime(napi_env env, napi_callback_info info) {
  napi_value result;

#ifdef OHOS_PLATFORM
  struct sysinfo sysInfo;
  if (sysinfo(&sysInfo) == 0) {
    napi_create_number(env, static_cast<double>(sysInfo.uptime), &result);
  } else {
    napi_create_number(env, 0, &result);
  }
#else
  napi_create_number(env, 0, &result);
#endif

  return result;
}

napi_value GetUserInfo(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  uid_t uid = getuid();
  struct passwd* pwd = getpwuid(uid);
  if (pwd) {
    napi_value name, passwd, uidVal, gid, gecos, dir, shell;
    napi_create_string_utf8(env, pwd->pw_name, NAPI_AUTO_LENGTH, &name);
    napi_create_string_utf8(env, pwd->pw_passwd, NAPI_AUTO_LENGTH, &passwd);
    napi_create_number(env, pwd->pw_uid, &uidVal);
    napi_create_number(env, pwd->pw_gid, &gid);
    napi_create_string_utf8(env, pwd->pw_gecos, NAPI_AUTO_LENGTH, &gecos);
    napi_create_string_utf8(env, pwd->pw_dir, NAPI_AUTO_LENGTH, &dir);
    napi_create_string_utf8(env, pwd->pw_shell, NAPI_AUTO_LENGTH, &shell);

    napi_set_named_property(env, result, "name", name);
    napi_set_named_property(env, result, "passwd", passwd);
    napi_set_named_property(env, result, "uid", uidVal);
    napi_set_named_property(env, result, "gid", gid);
    napi_set_named_property(env, result, "gecos", gecos);
    napi_set_named_property(env, result, "dir", dir);
    napi_set_named_property(env, result, "shell", shell);
  }
#else
  napi_value name, passwd, uid, gid, gecos, dir, shell;
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &name);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &passwd);
  napi_create_number(env, 0, &uid);
  napi_create_number(env, 0, &gid);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &gecos);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &dir);
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &shell);

  napi_set_named_property(env, result, "name", name);
  napi_set_named_property(env, result, "passwd", passwd);
  napi_set_named_property(env, result, "uid", uid);
  napi_set_named_property(env, result, "gid", gid);
  napi_set_named_property(env, result, "gecos", gecos);
  napi_set_named_property(env, result, "dir", dir);
  napi_set_named_property(env, result, "shell", shell);
#endif

  return result;
}

napi_value GetSystemTime(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

  time_t now = time(nullptr);
  struct tm* localTime = localtime(&now);
  struct tm* gmTime = gmtime(&now);

  napi_value timestamp, local, utc;
  napi_create_number(env, static_cast<double>(now), &timestamp);

  char localBuffer[100];
  strftime(localBuffer, sizeof(localBuffer), "%Y-%m-%d %H:%M:%S", localTime);
  napi_create_string_utf8(env, localBuffer, NAPI_AUTO_LENGTH, &local);

  char utcBuffer[100];
  strftime(utcBuffer, sizeof(utcBuffer), "%Y-%m-%d %H:%M:%S", gmTime);
  napi_create_string_utf8(env, utcBuffer, NAPI_AUTO_LENGTH, &utc);

  napi_set_named_property(env, result, "timestamp", timestamp);
  napi_set_named_property(env, result, "local", local);
  napi_set_named_property(env, result, "utc", utc);

  return result;
}

napi_value GetCPUInfo(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_array(env, &result);

#ifdef OHOS_PLATFORM
  FILE* fp = fopen("/proc/cpuinfo", "r");
  if (fp) {
    char line[256];
    int cpuIndex = -1;
    napi_value cpuObject = nullptr;

    while (fgets(line, sizeof(line), fp)) {
      if (strstr(line, "processor\t:")) {
        if (cpuObject) {
          napi_set_element(env, result, cpuIndex, cpuObject);
        }
        cpuIndex++;
        napi_create_object(env, &cpuObject);
      } else if (cpuObject) {
        char* colon = strchr(line, ':');
        if (colon) {
          *colon = '\0';
          char* key = line;
          char* value = colon + 1;

          // Trim whitespace
          while (*key && isspace(*key)) key++;
          while (*value && isspace(*value)) value++;
          char* end = value + strlen(value) - 1;
          while (end > value && isspace(*end)) *end-- = '\0';

          napi_value keyStr, valueStr;
          napi_create_string_utf8(env, key, NAPI_AUTO_LENGTH, &keyStr);
          napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &valueStr);
          napi_set_property(env, cpuObject, keyStr, valueStr);
        }
      }
    }

    if (cpuObject) {
      napi_set_element(env, result, cpuIndex, cpuObject);
    }

    fclose(fp);
  }
#endif

  return result;
}

napi_value GetNetworkInterfaces(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_object(env, &result);

#ifdef OHOS_PLATFORM
  FILE* fp = fopen("/proc/net/dev", "r");
  if (fp) {
    char line[256];
    // Skip header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
      char* colon = strchr(line, ':');
      if (colon) {
        *colon = '\0';
        char* ifname = line;
        // Trim whitespace
        while (*ifname && isspace(*ifname)) ifname++;
        char* end = ifname + strlen(ifname) - 1;
        while (end > ifname && isspace(*end)) *end-- = '\0';

        napi_value ifObject;
        napi_create_object(env, &ifObject);

        // Parse statistics
        unsigned long long rxBytes, rxPackets, rxErrors, rxDropped;
        unsigned long long txBytes, txPackets, txErrors, txDropped;
        sscanf(colon + 1, "%llu %llu %llu %llu %*u %*u %*u %*u %llu %llu %llu %llu",
               &rxBytes, &rxPackets, &rxErrors, &rxDropped,
               &txBytes, &txPackets, &txErrors, &txDropped);

        napi_value rxBytesVal, rxPacketsVal, rxErrorsVal, rxDroppedVal;
        napi_value txBytesVal, txPacketsVal, txErrorsVal, txDroppedVal;

        napi_create_number(env, static_cast<double>(rxBytes), &rxBytesVal);
        napi_create_number(env, static_cast<double>(rxPackets), &rxPacketsVal);
        napi_create_number(env, static_cast<double>(rxErrors), &rxErrorsVal);
        napi_create_number(env, static_cast<double>(rxDropped), &rxDroppedVal);
        napi_create_number(env, static_cast<double>(txBytes), &txBytesVal);
        napi_create_number(env, static_cast<double>(txPackets), &txPacketsVal);
        napi_create_number(env, static_cast<double>(txErrors), &txErrorsVal);
        napi_create_number(env, static_cast<double>(txDropped), &txDroppedVal);

        napi_set_named_property(env, ifObject, "rxBytes", rxBytesVal);
        napi_set_named_property(env, ifObject, "rxPackets", rxPacketsVal);
        napi_set_named_property(env, ifObject, "rxErrors", rxErrorsVal);
        napi_set_named_property(env, ifObject, "rxDropped", rxDroppedVal);
        napi_set_named_property(env, ifObject, "txBytes", txBytesVal);
        napi_set_named_property(env, ifObject, "txPackets", txPacketsVal);
        napi_set_named_property(env, ifObject, "txErrors", txErrorsVal);
        napi_set_named_property(env, ifObject, "txDropped", txDroppedVal);

        napi_value ifnameStr;
        napi_create_string_utf8(env, ifname, NAPI_AUTO_LENGTH, &ifnameStr);
        napi_set_property(env, result, ifnameStr, ifObject);
      }
    }

    fclose(fp);
  }
#endif

  return result;
}

napi_value GetLoadAverage(napi_env env, napi_callback_info info) {
  napi_value result;
  napi_create_array(env, &result);

#ifdef OHOS_PLATFORM
  double loadavg[3];
  if (getloadavg(loadavg, 3) == 3) {
    napi_value one, five, fifteen;
    napi_create_number(env, loadavg[0], &one);
    napi_create_number(env, loadavg[1], &five);
    napi_create_number(env, loadavg[2], &fifteen);

    napi_set_element(env, result, 0, one);
    napi_set_element(env, result, 1, five);
    napi_set_element(env, result, 2, fifteen);
  }
#endif

  return result;
}

napi_value GetHostname(napi_env env, napi_callback_info info) {
  char hostname[256];
  napi_value result;

#ifdef OHOS_PLATFORM
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    napi_create_string_utf8(env, hostname, NAPI_AUTO_LENGTH, &result);
  } else {
    napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &result);
  }
#else
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &result);
#endif

  return result;
}

napi_value GetPlatform(napi_env env, napi_callback_info info) {
  napi_value result;

#ifdef OHOS_PLATFORM
  napi_create_string_utf8(env, "OHOS", NAPI_AUTO_LENGTH, &result);
#elif defined(__linux__)
  napi_create_string_utf8(env, "Linux", NAPI_AUTO_LENGTH, &result);
#elif defined(_WIN32)
  napi_create_string_utf8(env, "Windows", NAPI_AUTO_LENGTH, &result);
#elif defined(__APPLE__)
  napi_create_string_utf8(env, "macOS", NAPI_AUTO_LENGTH, &result);
#else
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &result);
#endif

  return result;
}

napi_value GetArch(napi_env env, napi_callback_info info) {
  napi_value result;

#ifdef __aarch64__
  napi_create_string_utf8(env, "arm64", NAPI_AUTO_LENGTH, &result);
#elif defined(__arm__)
  napi_create_string_utf8(env, "arm", NAPI_AUTO_LENGTH, &result);
#elif defined(__x86_64__)
  napi_create_string_utf8(env, "x64", NAPI_AUTO_LENGTH, &result);
#elif defined(__i386__)
  napi_create_string_utf8(env, "x86", NAPI_AUTO_LENGTH, &result);
#else
  napi_create_string_utf8(env, "Unknown", NAPI_AUTO_LENGTH, &result);
#endif

  return result;
}

} // namespace

extern "C" {

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
    { "getOSInfo", nullptr, GetOSInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getMemoryInfo", nullptr, GetMemoryInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getDiskInfo", nullptr, GetDiskInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getProcessInfo", nullptr, GetProcessInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getEnvironmentVariable", nullptr, GetEnvironmentVariable, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "setEnvironmentVariable", nullptr, SetEnvironmentVariable, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getUptime", nullptr, GetUptime, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getUserInfo", nullptr, GetUserInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getSystemTime", nullptr, GetSystemTime, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getCPUInfo", nullptr, GetCPUInfo, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getNetworkInterfaces", nullptr, GetNetworkInterfaces, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getLoadAverage", nullptr, GetLoadAverage, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getHostname", nullptr, GetHostname, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getPlatform", nullptr, GetPlatform, nullptr, nullptr, nullptr, napi_default, nullptr },
    { "getArch", nullptr, GetArch, nullptr, nullptr, nullptr, napi_default, nullptr },
  };

  napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
  return exports;
}

NAPI_MODULE(native_module_system, Init)

}

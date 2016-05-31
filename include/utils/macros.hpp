#pragma once

#define writeln(s) std::cout << s << std::endl
#define writeln_ss(s) { std::stringstream ss; ss << s; writeln(ss.str()); std::stringstream s; }

#define ToStr(s) std::string(s)
#define IntToStr(s) std::to_string(s)

#define StrErrno() ToStr(std::strerror(errno))
#define StrErrnoCustom(s) ToStr(std::strerror(s))

#define StrSignal(sig) IntToStr(sig)
#define StrSignalC(sig) strsignal(sig)

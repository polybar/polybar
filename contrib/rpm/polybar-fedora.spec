Name: polybar	
Version: 3.3.0
Release: 1%{?dist}
Summary: helps users build beautiful and highly customizable status bars

License: MIT
URL: https://github.com/jaagr/%{name}	
Source0: %{url}/releases/download/%{version}/%{name}-%{version}.tar

BuildRequires: gcc-c++
BuildRequires: cmake
BuildRequires: ninja-build
BuildRequires: python2
BuildRequires: git
BuildRequires: pkgconfig(cairo)
BuildRequires: pkgconfig(xcb)
BuildRequires: pkgconfig(xcb-util)
BuildRequires: pkgconfig(xcb-proto)
BuildRequires: pkgconfig(xcb-image)
BuildRequires: pkgconfig(xcb-ewmh)
#Optional(module)
BuildRequires: pkgconfig(xcb-xrm)
BuildRequires: pkgconfig(xcb-cursor)
BuildRequires: pkgconfig(alsa)
BuildRequires: pkgconfig(libpulse)
BuildRequires: pkgconfig(jsoncpp)
BuildRequires: pkgconfig(libmpdclient)
BuildRequires: pkgconfig(libcurl)
BuildRequires: pkgconfig(libnl-genl-3.0)
BuildRequires: wireless-tools-devel
BuildRequires: i3-ipc

%description
Polybar aims to help users build beautiful and highly customizable status bars for their desktop environment, without the need of having a black belt in shell scripting.

The main purpose of Polybar is to help users create awesome status bars. It has built-in functionality to display information about the most commonly used services.

%prep
%setup -q -n %{name}
mkdir -p %{_target_platform}

%build
pushd %{_target_platform}
    cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX:PATH=%{_prefix} \
    -DBUILD_TESTS:BOOL=ON \
    ..
popd
%ninja_build -C %{_target_platform}

%install
%ninja_install -C %{_target_platform}

%check
pushd %{_target_platform}
    ctest --output-on-failure
popd

%files
%doc README.md
%license LICENSE

%{_bindir}/%{name}
%{_bindir}/%{name}-msg
%{_mandir}/man1/%{name}.1.gz
%{_datadir}/bash-completion/completions/%{name}
%{_datadir}/zsh/site-functions/_%{name}
%{_datadir}/zsh/site-functions/_%{name}_msg
%config(noreplace) %{_docdir}/%{name}/config

%changelog

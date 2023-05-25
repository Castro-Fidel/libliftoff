%define _unpackaged_files_terminate_build 1
%define _stripped_files_terminate_build 1
%set_verify_elf_method strict

Name: libliftoff
Version: 0.4.1
Release: alt1
Summary: Lightweight KMS plane library
Group: System/Libraries
License: MIT
Url: https://gitlab.freedesktop.org/emersion/libliftoff

# https://gitlab.freedesktop.org/emersion/libliftoff.git
Source: %name-%version.tar

BuildRequires(pre): meson
BuildRequires: libdrm-devel

%description
libliftoff eases the use of KMS planes from userspace without standing in your
way. Users create "virtual planes" called layers, set KMS properties on them,
and libliftoff will pick planes for these layers if possible.

%package devel
Summary: Lightweight KMS plane library development package
Group: Development/C
Requires: %name = %EVR

%description devel
libliftoff eases the use of KMS planes from userspace without standing in your
way. Users create "virtual planes" called layers, set KMS properties on them,
and libliftoff will pick planes for these layers if possible.

This package contains development files for libliftoff.

%prep
%setup

%build
%meson

%meson_build -v

%install
%meson_install

%files
%doc LICENSE
%doc README.md
%_libdir/*.so.*

%files devel
%_includedir/*
%_libdir/*.so
%_pkgconfigdir/*.pc

%changelog
* Thu May 25 2023 Mikhail Tergoev <fidel@altlinux.org> 0.4.1-alt1
- new version 0.4.1 (with rpmrb script)

* Tue Feb 22 2022 Aleksei Nikiforov <darktemplar@altlinux.org> 0.2.0-alt1
- Initial build for ALT.

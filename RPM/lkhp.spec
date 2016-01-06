Summary: Linux Kernel Hot-Patcher for i386.
Name: lkhp
Version: 1.0
Release: 1
Copyright: Commercial
Vendor: Wipro
Group: misc
BuildRoot: %{_topdir}/BUILD

%description
    This RPM installs the LKHP package.

%files
%defattr(755,root,root)
/usr/local/lkhp/LKHP.o
/usr/local/lkhp/dummy.c
/usr/local/lkhp/hop
/usr/local/lkhp/strap
/usr/sbin/lkhp
/usr/sbin/lkhp_setup
/usr/sbin/lkhp_unload
/usr/local/lkhp/include/sysdep.h
/usr/local/lkhp/data/System.map
%changelog

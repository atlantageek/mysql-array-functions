User Defined functions for mysql to support arrays
==================================================

These functions allow you to store arrays in a mysql blob and then do functions such as 
select maxarray(arrblob) from datatable group by location;

This is real useful when working with stored spectrum data because if you collect a lot of spectrum
samples over a period of time and want to create a peak hold then you could do it inside the sql instead of 
downloading all the data and then programatically generating a peak hold type trace.

You can also average your traces with avgarray() or generate a minhold type function with minarray.

To install
-----------
1. run make to generate the library
2. rename library libudf_arrayfunc.so  and put it in /lib/lib64
3. Then run the following commands in mysql to make functions available.
  * mysql -u root myql -e "create AGGREGATE FUNCTION avgarray RETURNS STRING SONAME 'libudf_arrayfunc.so';"
  * mysql -u root mysql -e "create AGGREGATE FUNCTION minarray RETURNS STRING SONAME 'libudf_arrayfunc.so';"
  * mysql -u root mysql -e "create AGGREGATE FUNCTION maxarray RETURNS STRING SONAME 'libudf_arrayfunc.so';"
4. Last but not least restart mysql
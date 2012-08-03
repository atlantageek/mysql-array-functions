User Defined functions for mysql to support arrays
==================================================

These functions allow you to store arrays in a mysql blob and then do functions such as 
select maxarray(arrblob) from datatable group by location;

This is real useful when working with stored spectrum data because if you collect a lot of spectrum
samples over a period of time and want to create a peak hold then you could do it inside the sql instead of 
downloading all the data and then programatically generating a peak hold type trace.

You can also average your traces with avgarray() or generate a minhold type function with minarray.



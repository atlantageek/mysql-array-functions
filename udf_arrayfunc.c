
#ifdef STANDARD
/* STANDARD is defined, don't use any mysql functions */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;	/* Microsofts 64 bit types */
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#if defined(MYSQL_SERVER)
#include <m_string.h>		/* To get strmov() */
#else
/* when compiled as standalone */
#include <string.h>
#define strmov(a,b) stpcpy(a,b)
#define bzero(a,b) memset(a,0,b)
#define memcpy_fixed(a,b,c) memcpy(a,b,c)
#endif
#endif
#include <mysql.h>
#include <ctype.h>

static pthread_mutex_t LOCK_hostname;

#ifdef HAVE_DLOPEN

#define MAXARRAY 501
struct array_data
{ 
	float arr[MAXARRAY];
	int datacount[MAXARRAY];
	int array_size;
};

/*Helper Functions */

/* We can have multiple methods to determine the value of each cell. But the interpolation loop is the same. So we developed a pointer to a function
 * that will calculate the value of each cell.  We will pass it the cell before the target value. The cell value after the target value and a weight.
 * For a min or max function it probably wont use the weight but a averaging function it will.
 */
void interpolate(float *data,int data_len,int data_start, int data_stop, int dest_len, int dest_start, int dest_stop, float (*interpolate_func)(float,float,float),float default_val, float *result, int *bitarray)
{
  int result_pos=0;
  int result_freq_pos=0;
  float float_pos;
  if (result == NULL)
  {
    fprintf(stderr, "No Space Made for result");
    exit(1);
  }
  if (bitarray == NULL)
  {
    fprintf(stderr, "No Space Made for bitarray");
    exit(1);
  }
  fprintf(stderr,"INTERPOLATE START %d - %d -> %d - %d",data_start, data_stop, dest_start, dest_stop);
  for (result_pos=0;result_pos < dest_len;result_pos++)
  {
    result_freq_pos=((dest_stop-dest_start)/(dest_len-1) * result_pos + dest_start);
    if ((result_freq_pos < data_start) || (result_freq_pos > data_stop))
    {
       result[result_pos]=default_val;
       bitarray[result_pos]=0;
    }
    else
    {
       float_pos=(float)(result_freq_pos-data_start)* ((float)(data_len-1)/(float)(data_stop-data_start));
       result[result_pos]=interpolate_func(data[(int)floor(float_pos)],data[(int)ceil(float_pos)],ceil(float_pos)-float_pos);
       bitarray[result_pos]=1;
    }
    
  }
  
}

float wgt_avg_closure(float val1,float val2,float wgt)
{
  float result=(float)(val1*wgt +val2*(1-wgt));
  return result;
}
float min_closure(float val1,float val2,float wgt)
{
  float result=(float)val1;
  if (val2 < val1)
  {
    result=val2;
  }
  return result;
}
float max_closure(float val1,float val2,float wgt)
{
  float result=val1;
  if (val2 > val1)
  {
    result=val2;
  }
  return result;
}
float total_closure(float val1,float val2,float wgt)
{
  return val1+val2;
}
void xxx_add(UDF_INIT* initid, UDF_ARGS* args,float(*interpolate_func)(float,float,float),float(*add_func)(float,float,float),float default_val)
{
  /*Create Default variables*/
  int i=0;
  struct array_data* data = (struct array_data*)initid->ptr;
  float default_value=100.0;
  /* Pull out Arguments */
  float *arr = ((float *)args->args[0]);
  int  array_size=0;
  int  data_start_freq=0;
  int  data_stop_freq=0;
  int  result_len=0;
  int  result_start_freq=0;
  int  result_stop_freq=0;
  float *interpolated_data;
  int *interpolated_bitarray;
  if ( args->arg_type[1]== INT_RESULT)
  {
     array_size = (int)*((longlong *)args->args[1]);
  }
  else {
     array_size = (int) *((double *)(args->args[1]));
  }
  if ( args->arg_type[2]== INT_RESULT)
  {
     data_start_freq = (int)*((longlong *)args->args[2]);
  }
  else {
     data_start_freq = (int) *((double *)args->args[2]);
  }
  if ( args->arg_type[3]== INT_RESULT)
  {
     data_stop_freq = (int)*((longlong *)args->args[3]);
  }
  else {
     data_stop_freq = (int) *((double *)args->args[3]);
  }
  if ( args->arg_type[4]== INT_RESULT)
  {
     result_len = (int)*((longlong *)args->args[4]);
  }
  else {
     result_len = (float) *((double *)args->args[4]);
  }
  if ( args->arg_type[5]== INT_RESULT)
  {
     result_start_freq = (int)*((longlong *)args->args[5]);
  }
  else {
     result_start_freq = (int) *((double *)args->args[5]);
  }
  if ( args->arg_type[6]== INT_RESULT)
  {
     result_stop_freq = (int)*((longlong *)args->args[6]);
  }
  else {
     result_stop_freq = (int) *((double *)args->args[6]);
  }
  /*Translate data into common range*/
  fprintf(stderr,"interpolate(arr, %%? , %d , %d , %d , %d , %d , func,%d)\n" ,data_start_freq,data_stop_freq,result_len,result_start_freq,result_stop_freq,default_val);
  interpolated_data=malloc(sizeof(float)*MAXARRAY); /*Interpolated_data will be freed at the end of this function */
  interpolated_bitarray=malloc(sizeof(int)*MAXARRAY); /*Interpolated_data will be freed at the end of this function */
  interpolate(arr,array_size,data_start_freq,data_stop_freq,result_len,result_start_freq,result_stop_freq,interpolate_func,default_val, interpolated_data,interpolated_bitarray);
  /*Increment count. Useful for averaging */
  for (i=0;i<array_size;i++)
  {
    if (interpolated_bitarray[i] == 1)
    {
      data->arr[i]=add_func(interpolated_data[i],data->arr[i],0);
      data->datacount[i]+=1;
    }
  }
  if (array_size > data->array_size)
  {
     data->array_size =array_size;
  }
  free(interpolated_data);
  free(interpolated_bitarray);
}

my_bool xxx_init(UDF_INIT *initid, UDF_ARGS *args, char *message, char *sql_func_name)
{
  int i;
  struct array_data *data;
  if (args->arg_count != 7 )
  {
    sprintf(message,"Wrong arguments to %s. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name,sql_func_name);
    return 1;
  }
  fprintf(stderr,"XXX ARRAY func name=%s\n", sql_func_name);
  fprintf(stderr,"XXX ARRAY func arg 0 %d\n", args->arg_type[0]);
  fprintf(stderr,"XXX ARRAY func arg 1 %d\n", args->arg_type[1]);
  fprintf(stderr,"XXX ARRAY func arg 2 %d\n", args->arg_type[2]);
  fprintf(stderr,"XXX ARRAY func arg 3 %d\n", args->arg_type[3]);
  fprintf(stderr,"XXX ARRAY func arg 4 %d\n", args->arg_type[4]);
  fprintf(stderr,"XXX ARRAY func arg 5 %d\n", args->arg_type[5]);
  fprintf(stderr,"XXX ARRAY func arg 6 %d\n", args->arg_type[6]);
  fprintf(stderr, "XXX ARRAY Init for %s(%d,%d,%d,%d,%d,%d,%d)\n",sql_func_name,args->arg_type[0],args->arg_type[1],args->arg_type[2],args->arg_type[3],args->arg_type[4],args->arg_type[5],args->arg_type[6]);
  if (args->arg_type[0] != STRING_RESULT) 
  {
    sprintf(message,"Array is not type string. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[1]== STRING_RESULT)
  {
    sprintf(message,"Array Size is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[2]== STRING_RESULT)
  {
    sprintf(message,"Start is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[3]== STRING_RESULT)
  {
    sprintf(message,"Stop is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[4]== STRING_RESULT)
  {
    sprintf(message,"Target Size is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[5]== STRING_RESULT)
  {
    sprintf(message,"Target Start is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  if ( args->arg_type[6]== STRING_RESULT)
  {
    sprintf(message,"Target Stop is not type int. Format is %s(array,array_size, start,stop,target_size,target_stop,target_stop)  ",sql_func_name);
    return 1;
  }
  
  if (!(data=(struct array_data *) malloc(sizeof(struct array_data)))) /* Freed in deinit*/
  {
    sprintf(message,"Couldn't allocate memory");
    fprintf (stderr,"%s\n",message);
    return 1;
  }
  for (i=0;i<MAXARRAY;i++)  /*TODO Should replace with a memfill*/
  {
     data->arr[i]=0.0;
     data->datacount[i]=0.0;
  }
  initid->ptr=(void *)data;
  initid->max_length=4*MAXARRAY*sizeof(float);
  return 0;
}

/*MinArray functions*/
my_bool minarray_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
   return xxx_init(initid, args, message, "minarray");
}
void minarray_deinit(UDF_INIT *initid __attribute__((unused)))
{
  free(initid->ptr);
}
void minarray_clear(UDF_INIT* initid, char* is_null __attribute__((unused)),
              char* message __attribute__((unused)))
{
  struct array_data* data = (struct array_data*)initid->ptr;
  int i;
  fprintf(stderr, "In Clear minarray\n");
  for (i=0;i<MAXARRAY;i++)  /*TODO Should replace with a memfill*/
  {
     data->datacount[i]=	0;
     data->arr[i]=1000;
  }

}
void minarray_add(UDF_INIT* initid, UDF_ARGS* args,
            char* is_null __attribute__((unused)),
            char* message __attribute__((unused)))
{
  fprintf(stderr, "MINARRAY arrayadd \n");
  xxx_add(initid,args,min_closure,min_closure,1000.0);
  fprintf(stderr, "Completed Add minarray\n");
}
void minarray_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message)
{
  fprintf(stderr, "MINARRAY arrayreset \n");
  minarray_clear(initid, is_null, message);
  minarray_add(initid, args, is_null, message);
}
char *minarray(UDF_INIT *initid, UDF_ARGS *args, char *result,
	       unsigned long *length, char *is_null, char *error)
{
    struct array_data* data	= (struct array_data*)initid->ptr;
    fprintf(stderr, "In minarray, returning min array.\n");
    *is_null =0;
    *length=MAXARRAY*sizeof(float);
    return (char *)data->arr;
}
/*MaxArray functions*/
my_bool maxarray_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
   return xxx_init(initid, args, message, "maxarray");
}
void maxarray_deinit(UDF_INIT *initid __attribute__((unused)))
{
  free(initid->ptr);
}
void maxarray_clear(UDF_INIT* initid, char* is_null __attribute__((unused)),
              char* message __attribute__((unused)))
{
  struct array_data* data = (struct array_data*)initid->ptr;
  int i;
  for (i=0;i<MAXARRAY;i++)  /*TODO Should replace with a memfill*/
  {
     data->datacount[i]=	0;
     data->arr[i]=-1000;
  }

}
void maxarray_add(UDF_INIT* initid, UDF_ARGS* args,
            char* is_null __attribute__((unused)),
            char* message __attribute__((unused)))
{
  xxx_add(initid,args,max_closure,max_closure,-32640);
}
void maxarray_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message)
{
  maxarray_clear(initid, is_null, message);
  maxarray_add(initid, args, is_null, message);
}
char *maxarray(UDF_INIT *initid, UDF_ARGS *args, char *result,
	       unsigned long *length, char *is_null, char *error)
{
    struct array_data* data	= (struct array_data*)initid->ptr;
    *is_null =0;
    *length=MAXARRAY*sizeof(float);
    return (char *)data->arr;
}
/*AvgArray functions*/
my_bool avgarray_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
   return xxx_init(initid, args, message, "avgarray");
}
void avgarray_deinit(UDF_INIT *initid __attribute__((unused)))
{
  free(initid->ptr);
}
void avgarray_clear(UDF_INIT* initid, char* is_null __attribute__((unused)),
              char* message __attribute__((unused)))
{
  int i=0;
  struct array_data* data = (struct array_data*)initid->ptr;
  for (i=0;i<MAXARRAY;i++)  /*TODO Should replace with a memfill*/
  {
     data->datacount[i]=	0;
     data->arr[i]=0.0;
  }
  /*THIS memset is breaking the system.  Setting too many fileds to zero maybe?*/
  /*memset(data->arr,0x0080,MAXARRAY*sizeof(float)); /* memset casts 0x0080 to unsigned char and then Fills the array with a 0x80.  */
                                                 /* When we pull it out we will get -127 for a byte or for a signed short we get -32640*/

}
void avgarray_add(UDF_INIT* initid, UDF_ARGS* args,
            char* is_null __attribute__((unused)),
            char* message __attribute__((unused)))
{
  xxx_add(initid,args,wgt_avg_closure,total_closure,-32640);
  fprintf(stderr, "Completed Add avgarray\n");
}
void avgarray_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* message)
{
  avgarray_clear(initid, is_null, message);
  avgarray_add(initid, args, is_null, message);
}
char *avgarray(UDF_INIT *initid, UDF_ARGS *args, char *result,
	       unsigned long *length, char *is_null, char *error)
{
    int i=0;
    int arraylen=0;
    struct array_data* data;
   
    data	= (struct array_data*)initid->ptr;
    *is_null =0;
    for(i=0;i<data->array_size;i++)
    {
      data->arr[i]=data->arr[i]/(float)data->datacount[i];
    }
   *length=MAXARRAY*sizeof(float);
    return (char *)data->arr;
}



#endif /* HAVE_DLOPEN */

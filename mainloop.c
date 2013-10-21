/*************************************************************************
    > File Name: /home/administrator/Documents/python/pulse/pypa/mainloop.c
    > Author: onerhao
    > Mail: haodu@hustunique.com
    > Created Time: 2013年10月11日 星期五 14时46分06秒
 ************************************************************************/

#include <Python.h>
#include "structmember.h"
#include <pulse/pulseaudio.h>
#include <pulse/mainloop.h>

#define INT(v) PyInt_FromLong(v)
#define STRING(v) PyString_FromLong(v)

#define DECREF(v,tmp) \
	tmp=(v); \
	(v)=NULL;          \
	Py_XDECREF(tmp);   \

#define DICT_NEW(v) \
		{ \
			(v)=PyDict_New(); \
			if(!(v)) \
			{ \
				Py_DECREF(self); \
				return NULL; \
			} \
		}

#define LIST_NEW(v) \
		{ \
			(v)=PyList_New(0); \
			if(!(v)) \
			{ \
				Py_DECREF(self); \
				return NULL; \
			} \
		}

#define PYSTRING_FROMSTRING(v) \
	(v)?PyString_FromString(v):PyString_FromStringAndSize(NULL,0)


typedef struct _pa
{
    PyObject_HEAD
    PyObject *dict; //python attributes dictionary
    pa_mainloop *pa_ml;
    pa_mainloop_api *pa_mlapi;
    pa_context *pa_ctx;
    pa_operation   *pa_op;

    PyObject *server_info;
    PyObject *cards;
    PyObject *sinks;
    PyObject *sources;
    PyObject *clients;
    PyObject *sink_inputs;
    PyObject *source_outputs;
    PyObject *input_ports;
    PyObject *output_ports;
} pa;

static int pa_traverse(pa *self,visitproc visit,void *arg)
{
    int vret;
#undef VISIT
#define VISIT(v) \
	if((v)) { \
		vret=visit((v),arg); \
		if(vret != 0) \
			return vret; \
	}

    VISIT(self->server_info);
    VISIT(self->cards);
    VISIT(self->sources);
    VISIT(self->sinks);
    VISIT(self->input_ports);
    VISIT(self->output_ports);


    return 0;
}

static int pa_clear(pa *self)
{
    PyObject *tmp;

    if(self->pa_op)
    {
        pa_operation_unref(self->pa_op);
        self->pa_op=NULL;
    }

    if(self->pa_ctx)
    {
        pa_context_disconnect(self->pa_ctx);
        pa_context_unref(self->pa_ctx);
        self->pa_ctx=NULL;
    }

    if(self->pa_ml)
    {
        pa_mainloop_free(self->pa_ml);
        self->pa_ml=NULL;
    }

    self->pa_mlapi=NULL;

    DECREF(self->server_info,tmp);
    DECREF(self->cards,tmp);
    DECREF(self->sinks,tmp);
    DECREF(self->sources,tmp);
    DECREF(self->clients,tmp);
    DECREF(self->sink_inputs,tmp);
    DECREF(self->source_outputs,tmp);
    DECREF(self->input_ports,tmp);
    DECREF(self->output_ports,tmp);
    return 0;
}

static void pa_dealloc(pa *self)
{
    pa_clear(self);
    self->ob_type->tp_free((PyObject*)self);
    fprintf(stderr,"object deleted\n");
    return;
}

static PyObject * pa_new(PyTypeObject *type,PyObject *args,PyObject *kwds)
{
    pa *self;
    self=(pa*)type->tp_alloc(type,0);
    if(self!=NULL)
    {
        self->pa_ml=NULL;
        self->pa_mlapi=NULL;
        self->pa_ctx=NULL;
        self->pa_op=NULL;


        DICT_NEW(self->dict);
        DICT_NEW(self->server_info);
        DICT_NEW(self->cards);
        LIST_NEW(self->sinks);
        LIST_NEW(self->sources);
        LIST_NEW(self->clients);
        LIST_NEW(self->sink_inputs);
        LIST_NEW(self->source_outputs);
        LIST_NEW(self->input_ports);
        LIST_NEW(self->output_ports);
    }
    else
    {
        return NULL;
    }

    return (PyObject*)self;
}


static int pa_init(pa *self,PyObject *args,PyObject *kwds)
{
    //init function can be called multiple times,extra caution
    //with assigning the new values.


    /*PyObject *server_info=NULL,*card=NULL,*tmp;
    static char *kwlist[]={"first","last","number",NULL};

    if(! PyArg_ParseTupleAndKeywords(args,kwds,"|OOi",kwlist,
    			&server_info,&last,
    			&self->number))
    	return -1;

    }
    */

	if(self->pa_op)
	{
		pa_operation_unref(self->pa_op);
		self->pa_op=NULL;
	}
	if(self->pa_ctx)
	{
		pa_context_disconnect(self->pa_ctx);
		pa_context_unref(self->pa_ctx);
		self->pa_ctx=NULL;
	}
	if(self->pa_ml)
    {
        //to reassign,free first
        pa_mainloop_free(self->pa_ml);
		self->pa_ml=NULL;
    }

    self->pa_ml=pa_mainloop_new();
    if(!self->pa_ml)
    {
        perror("pa_mainloop_new()");
        return -1;
    }

    self->pa_mlapi=pa_mainloop_get_api(self->pa_ml);
    if(!self->pa_mlapi)
    {
        perror("pa_mainloop_get_api()");
        return -1;
    }

    self->pa_ctx=pa_context_new(self->pa_mlapi,"python-pulseaudio");
    if(!self->pa_ctx)
    {
        perror("pa_context_new()");
        return -1;
    }

    printf( "Object initialized\n");
    return 0;
}

static PyMemberDef pa_members[]=
{
    {
        "dict",T_OBJECT_EX,offsetof(pa,dict),0,
        "attribute dictionary"
    },
    {
        "server_info",T_OBJECT_EX,offsetof(pa,server_info),0,
        "pulseaudio server info"
    },
    {
        "cards",T_OBJECT_EX,offsetof(pa,cards),0,
        "cards"
    },
    {
        "sinks",T_OBJECT_EX,offsetof(pa,sinks),0,
        "output devices"
    },
    {
        "sources",T_OBJECT_EX,offsetof(pa,sources),0,
        "input devices"
    },
    {
        "clients",T_OBJECT_EX,offsetof(pa,clients),0,
        "clients"
    },
    {
        "sink_inputs",T_OBJECT_EX,offsetof(pa,sink_inputs),0,
        "list of stream that is connected to an output device,i.e. an input for a sink"
    },
    {
        "source_outputs",T_OBJECT_EX,offsetof(pa,source_outputs),0,
        "list of stream that is connected to an input device"
    },
    {NULL}    /* Sentinel */
};

typedef struct pa_devicelist
{
    uint8_t initialized;
    char name[512];
    uint32_t index;
    char description[256];
} pa_devicelist_t;

static PyObject *pa_init_context(pa *self);
static PyObject *pa_get_server_info(pa *self);
static PyObject *pa_get_card_list(pa *self);
static PyObject *pa_get_device_list(pa *self);
static PyObject *pa_get_client_list(pa *self);
static PyObject *pa_get_sink_input_list(pa *self);
static PyObject *pa_get_source_output_list(pa *self);
static PyObject *pa_set_sink_input_mute(pa *self,PyObject *args);
static PyObject* pa_get_sink_input_index_by_pid(pa *self,PyObject *args);
static PyObject* pa_set_sink_input_mute_by_pid(pa *self,PyObject *args);

void pa_state_cb(pa_context *c,void *userdata);
void pa_serverinfo_cb(pa_context *c, const pa_server_info*i, void *userdata);
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l,
                      int eol, void *userdata);
void pa_clientlist_cb(pa_context *c, const pa_client_info*i,
                      int eol, void *userdata);
void pa_sink_input_cb(pa_context *c,const pa_sink_input_info *i,
                      int eol,void *userdata);
void pa_source_output_cb(pa_context *c, const pa_source_output_info *i,
                         int eol, void *userdata);
void pa_cards_cb(pa_context *c, const pa_card_info*i, int eol, void *userdata);

void pa_set_sink_input_mute_cb(pa_context *c,int success,void *userdata);

//utils
PyObject *pa_dict_from_cvolume(pa_cvolume *c);





static PyMethodDef pa_methods[]=
{
	{
		"get_server_info",(PyCFunction)pa_get_server_info,METH_VARARGS,
		"get the server information."
	},
    {
        "get_cards",(PyCFunction)pa_get_card_list,METH_VARARGS,
        "return the information about all the cards."
    },
    {
        "get_devicelist",(PyCFunction)pa_get_device_list,METH_VARARGS,
        "Return lists of sinks and sources"
    },
    {
        "get_clients",(PyCFunction)pa_get_client_list,METH_VARARGS,
        "return the list of clients"
    },
    {
        "get_sink_inputs",(PyCFunction)pa_get_sink_input_list,METH_VARARGS,
        "return the list of sink inputs."
    },
    {
        "get_source_outputs",(PyCFunction)pa_get_source_output_list,METH_VARARGS,
        "return the list of source outputs."
    },
    {
        "set_sink_input_mute",(PyCFunction)pa_set_sink_input_mute,METH_VARARGS,
        "set or unset the sink input mute."
    },
	{
		"get_sink_input_index_by_pid",(PyCFunction)pa_get_sink_input_index_by_pid,METH_VARARGS,
		"get a sink input's index by its process id"
	},
	{
		"set_sink_input_mute_by_pid",(PyCFunction)pa_set_sink_input_mute_by_pid,METH_VARARGS,
		"set a sink input mute by pid"
	},
    {NULL}  /* Sentinel */
};

static PyMethodDef module_methods[]=
{
    {NULL}  /* Sentinel */
};

static PyObject *pa_init_context(pa *self)
{
	if(self->pa_op)
	{
		pa_operation_unref(self->pa_op);
		self->pa_op=NULL;
	}
	if(self->pa_ctx)
	{
		pa_context_disconnect(self->pa_ctx);
		pa_context_unref(self->pa_ctx);
		self->pa_ctx=NULL;
	}
	if(self->pa_ml)
    {
        pa_mainloop_free(self->pa_ml);
		self->pa_ml=NULL;
    }
    self->pa_ml=pa_mainloop_new();
    if(!self->pa_ml)
    {
        perror("pa_mainloop_new()");
        return -1;
    }

    self->pa_mlapi=pa_mainloop_get_api(self->pa_ml);
    if(!self->pa_mlapi)
    {
        perror("pa_mainloop_get_api()");
        return -1;
    }

    self->pa_ctx=pa_context_new(self->pa_mlapi,"python-pulseaudio");
    if(!self->pa_ctx)
    {
        perror("pa_context_new()");
        return Py_BuildValue("i",-1);
    }

	return Py_BuildValue("i",0);
}

static PyObject *pa_get_server_info(pa *self)
{
    int pa_ready = 0;
    int state = 0;

	if(self->server_info)
	{
		PyDict_Clear(self->server_info);
	}
	else
	{
		self->server_info=PyDict_New();
	}

    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    for (;;)
    {
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 0, NULL);
            continue;
        }
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
            pa_context_unref(self->pa_ctx);
            pa_mainloop_free(self->pa_ml);
            self->pa_ctx=NULL;
            self->pa_mlapi=NULL;
            self->pa_ml=NULL;
            return Py_BuildValue("i",-1);
        }
        switch (state)
        {
        case 0:
            self->pa_op = pa_context_get_server_info(self->pa_ctx, pa_serverinfo_cb, self);
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);
                pa_context_disconnect(self->pa_ctx);
                pa_context_unref(self->pa_ctx);
                pa_mainloop_free(self->pa_ml);
                self->pa_op=NULL;
                self->pa_ctx=NULL;
                self->pa_mlapi=NULL;
                self->pa_ml=NULL;
				pa_init_context(self);
				return Py_BuildValue("i",0);
            }
            break;
        default:
            fprintf(stderr, "in state %d\n", state);
            return Py_BuildValue("i",-1);
        }
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }
    return Py_BuildValue("i",0);
}

static PyObject *pa_get_card_list(pa *self)
{
    int pa_ready = 0;
    int state = 0;
    PyObject *tmp=NULL;

    DECREF(self->cards,tmp);
    LIST_NEW(self->cards);

    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    for (;;)
    {
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 0, NULL);
            continue;
        }
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
            pa_context_unref(self->pa_ctx);
            pa_mainloop_free(self->pa_ml);
            self->pa_ctx=NULL;
            self->pa_mlapi=NULL;
            self->pa_ml=NULL;
            return Py_BuildValue("i",-1);
        }
        switch (state)
        {
        case 0:
            self->pa_op = pa_context_get_card_info_list(self->pa_ctx, pa_cards_cb, self);
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);
                pa_context_disconnect(self->pa_ctx);
                pa_context_unref(self->pa_ctx);
                pa_mainloop_free(self->pa_ml);
                self->pa_op=NULL;
                self->pa_ctx=NULL;
                self->pa_mlapi=NULL;
                self->pa_ml=NULL;
				pa_init_context(self);
				return Py_BuildValue("i",0);
            }
            break;
        default:
            fprintf(stderr, "in state %d\n", state);
            return Py_BuildValue("i",-1);
        }
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }
    return Py_BuildValue("i",0);
}

static PyObject *pa_get_device_list(pa *self)
{
    // We'll need these state variables to keep track of our requests
    int state = 0;
    int pa_ready = 0;
    PyObject *tmp=self->sinks;

    DECREF(self->sinks,tmp);
    LIST_NEW(self->sinks);
    DECREF(self->sources,tmp);
    LIST_NEW(self->sources);

    fprintf(stderr,"before connection\n");

    // This function connects to the pulse server
    pa_context_connect(self->pa_ctx, NULL, 0, NULL);

    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    // Now we'll enter into an infinite loop until we get the data we receive
    // or if there's an error
    for (;;)
    {
        // We can't do anything until PA is ready, so just iterate the mainloop
        // and continue
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 1, NULL);
            continue;
        }
        // We couldn't get a connection to the server, so exit out
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
            //pa_context_unref(self->pa_ctx);
            //pa_mainloop_free(self->pa_ml);
            pa_context_unref(self->pa_ctx);
            pa_mainloop_free(self->pa_ml);
            self->pa_op=NULL;
            self->pa_ctx=NULL;
            self->pa_mlapi=NULL;
            self->pa_ml=NULL;

            Py_INCREF(Py_False);
            return Py_False;
            //This object has no methods,it needs to be treated just like any
            //other objects with respect to reference counts;
        }
        // At this point, we're connected to the server and ready to make
        // requests
        switch (state)
        {
            // State 0: we haven't done anything yet
        case 0:
            // This sends an operation to the server.  pa_sinklist_info is
            // our callback function and a pointer to our devicelist will
            // be passed to the callback The operation ID is stored in the
            // pa_op variable
            self->pa_op = pa_context_get_sink_info_list(self->pa_ctx,
                          pa_sinklist_cb,
                          self);
            // Update state for next iteration through the loop
            state++;
            break;
        case 1:
            // Now we wait for our operation to complete.  When it's
            // complete our pa_output_devicelist is filled out, and we move
            // along to the next state
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);

                // Now we perform another operation to get the source
                // (input device) list just like before.  This time we pass
                // a pointer to our input structure
                self->pa_op = pa_context_get_source_info_list(self->pa_ctx,
                              pa_sourcelist_cb,
                              self);
                // Update the state so we know what to do next
                state++;
            }
            break;
        case 2:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                // Now we're done, clean up and disconnect and return
                pa_operation_unref(self->pa_op);
                pa_context_disconnect(self->pa_ctx);
                pa_context_unref(self->pa_ctx);
                pa_mainloop_free(self->pa_ml);
                self->pa_op=NULL;
                self->pa_ctx=NULL;
                self->pa_mlapi=NULL;
                self->pa_ml=NULL;
				pa_init_context(self);
                Py_INCREF(Py_True);
                return Py_True;
            }
            break;
        default:
            // We should never see this state
            fprintf(stderr, "in state %d\n", state);
            Py_INCREF(Py_True);
            return Py_True;
        }
        // Iterate the main loop and go again.  The second argument is whether
        // or not the iteration should block until something is ready to be
        // done.  Set it to zero for non-blocking.
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *pa_get_client_list(pa *self)
{
    // We'll need these state variables to keep track of our requests
    int state = 0;
    int pa_ready = 0;
    PyObject *tmp=NULL;

    DECREF(self->clients,tmp);
    LIST_NEW(self->clients);


    // This function connects to the pulse server
    pa_context_connect(self->pa_ctx, NULL, 0, NULL);

    // This function defines a callback so the server will tell us it's state.
    // Our callback will wait for the state to be ready.  The callback will
    // modify the variable to 1 so we know when we have a connection and it's
    // ready.
    // If there's an error, the callback will set pa_ready to 2
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    // Now we'll enter into an infinite loop until we get the data we receive
    // or if there's an error
    for (;;)
    {
        // We can't do anything until PA is ready, so just iterate the mainloop
        // and continue
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 1, NULL);
            continue;
        }
        // We couldn't get a connection to the server, so exit out
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
            self->pa_op=NULL;
            self->pa_ctx=NULL;
            self->pa_mlapi=NULL;
            self->pa_ml=NULL;

            Py_INCREF(Py_False);
            return Py_False;
            //This object has no methods,it needs to be treated just like any
            //other objects with respect to reference counts;
        }
        // At this point, we're connected to the server and ready to make
        // requests
        switch (state)
        {
            // State 0: we haven't done anything yet
        case 0:
            // This sends an operation to the server.  pa_sinklist_info is
            // our callback function and a pointer to our devicelist will
            // be passed to the callback The operation ID is stored in the
            // pa_op variable
            self->pa_op = pa_context_get_client_info_list(self->pa_ctx,
                          pa_clientlist_cb,
                          self);
            // Update state for next iteration through the loop
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                // Now we're done, clean up and disconnect and return
                pa_operation_unref(self->pa_op);
                self->pa_op=NULL;
                pa_context_disconnect(self->pa_ctx);
                self->pa_op=NULL;
                self->pa_ctx=NULL;
                self->pa_mlapi=NULL;
                self->pa_ml=NULL;
				pa_init_context(self);
                //pa_context_unref(self->pa_ctx);
                //pa_mainloop_free(self->pa_ml);
                Py_INCREF(Py_True);
                return Py_True;
            }
            break;
        default:
            // We should never see this state
            fprintf(stderr, "in state %d\n", state);
            Py_INCREF(Py_True);
            return Py_True;
        }
        // Iterate the main loop and go again.  The second argument is whether
        // or not the iteration should block until something is ready to be
        // done.  Set it to zero for non-blocking.
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *pa_get_sink_input_list(pa *self)
{
    int pa_ready = 0;
    int state = 0;
    PyObject *tmp=NULL;

    DECREF(self->sink_inputs,tmp);
    LIST_NEW(self->sink_inputs);

    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    for (;;)
    {
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 1, NULL);
            continue;
        }
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
            self->pa_op=NULL;
            self->pa_ctx=NULL;
            self->pa_mlapi=NULL;
            self->pa_ml=NULL;
            Py_INCREF(Py_False);
            return Py_False;
        }
        switch (state)
        {
        case 0:
            self->pa_op = pa_context_get_sink_input_info_list(self->pa_ctx, pa_sink_input_cb, self);
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);
                self->pa_op=NULL;
                pa_context_disconnect(self->pa_ctx);
 				self->pa_op=NULL;
				self->pa_ctx=NULL;
				self->pa_mlapi=NULL;
				self->pa_ml=NULL;
				pa_init_context(self);
				Py_INCREF(Py_True);
				return Py_True;
			}
				break;
        default:
            fprintf(stderr, "in state %d\n", state);
            Py_INCREF(Py_True);
            return Py_True;
        }
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }
    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *pa_get_source_output_list(pa *self)
{
    int pa_ready = 0;
    int state = 0;
    PyObject *tmp=NULL;

    DECREF(self->sink_inputs,tmp);
    LIST_NEW(self->sink_inputs);

    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    for (;;)
    {
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 1, NULL);
            continue;
        }
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
 				self->pa_op=NULL;
				self->pa_ctx=NULL;
				self->pa_mlapi=NULL;
				self->pa_ml=NULL;
           return Py_BuildValue("i",-1);
        }
        switch (state)
        {
        case 0:
            self->pa_op = pa_context_get_source_output_info_list(self->pa_ctx, pa_source_output_cb, self);
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);
                self->pa_op=NULL;
                pa_context_disconnect(self->pa_ctx);
 				self->pa_op=NULL;
				self->pa_ctx=NULL;
				self->pa_mlapi=NULL;
				self->pa_ml=NULL;
				pa_init_context(self);
				return Py_BuildValue("i",0);
            }
            break;
        default:
            fprintf(stderr, "in state %d\n", state);
            return Py_BuildValue("i",-1);
        }
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }
    return Py_BuildValue(0);
}

static PyObject* pa_set_sink_input_mute(pa *self,PyObject *args)
{
    int pa_ready = 0;
    int state = 0;
    int index,mute;

    if(!PyArg_ParseTuple(args,"ii",&index,&mute))
    {
        fprintf(stderr,"sink input index and mute flag are needed\n");
        return NULL;
    }

    pa_context_connect(self->pa_ctx, NULL, 0, NULL);
    pa_context_set_state_callback(self->pa_ctx, pa_state_cb, &pa_ready);

    for (;;)
    {
        if (pa_ready == 0)
        {
            pa_mainloop_iterate(self->pa_ml, 1, NULL);
            continue;
        }
        if (pa_ready == 2)
        {
            pa_context_disconnect(self->pa_ctx);
 				self->pa_op=NULL;
				self->pa_ctx=NULL;
				self->pa_mlapi=NULL;
				self->pa_ml=NULL;
			return Py_BuildValue("i",-1);
        }
        switch (state)
        {
        case 0:
            self->pa_op=pa_context_set_sink_input_mute(self->pa_ctx,index,mute,pa_set_sink_input_mute_cb,NULL);
            state++;
            break;
        case 1:
            if (pa_operation_get_state(self->pa_op) == PA_OPERATION_DONE)
            {
                pa_operation_unref(self->pa_op);
                self->pa_op=NULL;
                pa_context_disconnect(self->pa_ctx);
 				self->pa_op=NULL;
				self->pa_ctx=NULL;
				self->pa_mlapi=NULL;
				self->pa_ml=NULL;
				pa_init_context(self);
	            return Py_BuildValue("i",0);
            }
            break;
        default:
            fprintf(stderr, "in state %d\n", state);
            return Py_BuildValue("i",-1);
        }
        pa_mainloop_iterate(self->pa_ml, 1, NULL);
    }
    return Py_BuildValue("i",0);
}

static PyObject* pa_get_sink_input_index_by_pid(pa *self,PyObject *args)
{
	if(!self)
	{
		fprintf(stderr,"NULL object pointer\n");
		return Py_BuildValue("i",-1);
	}
	PyObject *dict=NULL,*tmp;
	int i,l,index=-1;
	pid_t kpid,pid=-1;

	if(!PyArg_ParseTuple(args,"ii",&kpid,&i))
	{
		fprintf(stderr,"Expect a integer argument!\n");
		return Py_BuildValue("i",-1);
	}

	if(!self->sink_inputs)
	{
		//empty sink_inputs slot yet,update it first
		pa_get_sink_input_list(self);
	}
	if(!self->sink_inputs)
	{
		fprintf(stderr,"No sink inputs detected yet\n");
		return Py_BuildValue("i",-1);
	}

	l=PyList_Size(self->sink_inputs);
	for(i=0;i<l;i++)
	{
		dict=PyList_GetItem(self->sink_inputs,i);
		tmp=PyDict_GetItemString(dict,"application.process.id");
		pid=atoi(PyString_AsString(tmp));
		if(kpid==pid && PyErr_Occurred()==NULL)
		{
			index=PyInt_AsLong(PyDict_GetItemString(dict,"index"));
			return Py_BuildValue("i",index);
		}
	}

	fprintf(stderr,"No matching pid detected\n");
	return Py_BuildValue("i",-1);
}

static PyObject* pa_set_sink_input_mute_by_pid(pa *self,PyObject *args)
{
	PyObject *index_py;
	PyObject *tuple=NULL;
	int index,pid,mute;
	if(!self)
	{
		fprintf(stderr,"NULL object pointer\n");
		Py_INCREF(Py_False);
		return Py_False;
	}

	if(!PyArg_ParseTuple(args,"ii",&pid,&mute))
	{
		fprintf(stderr,"pid and mute value are expeted\n");
		Py_INCREF(Py_False);
		return Py_False;
	}

	index_py=pa_get_sink_input_index_by_pid(self,args);
	index=PyInt_AsLong(index_py);
	if(index!=-1)
	{
		tuple=PyTuple_New(2);
		PyTuple_SetItem(tuple,0,Py_BuildValue("i",index));
		PyTuple_SetItem(tuple,1,Py_BuildValue("i",mute));
		return pa_set_sink_input_mute(self,tuple);
	}
	else
	{
		Py_INCREF(Py_False);
		return Py_False;
	}
}

static PyTypeObject paType=
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pypa.pa",             /*tp_name*/
    sizeof(pa),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)pa_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    "pa objects",           /* tp_doc */
    (traverseproc)pa_traverse,		               /* tp_traverse */
    (inquiry)pa_clear,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    pa_methods,             /* tp_methods */
    pa_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pa_init,      /* tp_init */
    0,                         /* tp_alloc */
    pa_new,                 /* tp_new */
};


//higher level apis for manipulating pulseaudio
//



/*********CALLBACK**************/

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata)
{
    pa_context_state_t state;
    int *pa_ready = userdata;

    state = pa_context_get_state(c);
    switch  (state)
    {
        // There are just here for reference
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        *pa_ready = 2;
        break;
    case PA_CONTEXT_READY:
        *pa_ready = 1;
        break;
    }
}

void pa_serverinfo_cb(pa_context *c, const pa_server_info*i, void *userdata)
{
    pa *self= userdata;
    PyObject *dict=PyDict_New();

    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }

    PyDict_SetItemString(dict,"user_name",PYSTRING_FROMSTRING(i->user_name));
    PyDict_SetItemString(dict,"host_name",PYSTRING_FROMSTRING(i->host_name));
    PyDict_SetItemString(dict,"server_version",PYSTRING_FROMSTRING(i->server_version));
    PyDict_SetItemString(dict,"server_name",PYSTRING_FROMSTRING(i->server_name));
    PyDict_SetItemString(dict,"default_sink_name",PYSTRING_FROMSTRING(i->default_sink_name));
	PyDict_SetItemString(dict,"default_source_name",PYSTRING_FROMSTRING(i->default_source_name));
	PyDict_SetItemString(dict,"cookie",PyInt_FromLong(i->cookie));

    PyList_Append(self->server_info,dict);

	return;
}

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata)
{
    pa *self= userdata;
    PyObject *dict=PyDict_New();
    pa_sink_port_info **ports  = NULL;
    pa_sink_port_info *port = NULL;
    pa_cvolume volume;
    /*volume = malloc(sizeof(pa_cvolume));*/
    /*memcpy(&volume, &l->volume, sizeof(pa_cvolume));*/
    memset(&volume, 0, sizeof(pa_cvolume));
    int i = 0;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0)
    {
        return;
    }

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.  You could make this dynamically allocate
    // space for the device list, but this is a simple example.

    //const char *key;
    //PyObject *val;
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }

    PyDict_SetItemString(dict,"index",PyInt_FromLong(l->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(l->name));
    PyDict_SetItemString(dict,"description",PYSTRING_FROMSTRING(l->description));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(l->driver));
    PyDict_SetItemString(dict,"mute",PyInt_FromLong(l->mute));
    PyList_Append(self->sinks,dict);

    /*pa_cvolume_set(&l->volume, 0, 1000);*/
    /*pa_cvolume_set(&l->volume, 1, 5000);*/
    /*pa_cvolume_set(volume, 1, 1000);*/
    /*pa_cvolume_set(volume, 2, 5000);*/
    //pa_context_set_sink_volume_by_name(pa_context *c, const char *name, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);
    //pa_context_set_sink_volume_by_index(pa_context *c, uint32_t idx, const pa_cvolume *volume, pa_context_success_cb_t cb, void *userdata);
    volume.channels = l->volume.channels;
    for (i = 0; i < l->volume.channels; i++)
    {
        volume.values[i] = 15000;
        printf("\tvolume %d is %d\n", i, volume.values[i]);
    }
    pa_cvolume_set_balance(&volume, &l->channel_map, 0.5);
    /*volume.values[0] = 1000;*/
    /*volume.values[1] = 5000;*/
    /*l->volume.values[0] = 1000;*/
    /*l->volume.values[1] = 5000;*/
    /*pa_context_set_sink_volume_by_name(c, l->name,  &l->volume, NULL, NULL);*/
    pa_context_set_sink_volume_by_index(c, l->index,  &volume, NULL, NULL);
    /*free(volume);*/
    for (i = 0; i < l->channel_map.channels; i++)
    {
        printf("DEBUG channel map %d, volume:%d\n", l->channel_map.map[i], l->volume.values[i]);
    }
    ports = l->ports;
    for (i = 0; i < l->n_ports; i++)
    {
        port = ports[i];
        printf("DEBUG %s %s\n", port->name, port->description);
    }
    printf("sink------------------------------\n");
}

// See above.  This callback is pretty much identical to the previous
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata)
{
    pa *self= userdata;
    PyObject *dict=PyDict_New();
    pa_source_port_info **ports = NULL;
    pa_source_port_info *port = NULL;
    int i = 0;

    if (eol > 0)
    {
        return;
    }
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }

    PyDict_SetItemString(dict,"index",PyInt_FromLong(l->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(l->name));
    PyDict_SetItemString(dict,"description",PYSTRING_FROMSTRING(l->description));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(l->driver));
    PyDict_SetItemString(dict,"mute",PyInt_FromLong(l->mute));
    PyList_Append(self->sources,dict);

    ports = l->ports;
    printf("map can balance %d\n", pa_channel_map_can_balance(&l->channel_map));
    for (i = 0; i < l->n_ports; i++)
    {
        port = ports[i];
        printf("DEBUG %s %s\n", port->name, port->description);
    }
    printf("source------------------------------\n");
}
void pa_clientlist_cb(pa_context *c, const pa_client_info *i,
                      int eol, void *userdata)
{
    if (eol > 0)
    {
        printf("End of sinks\n");
        return;
    }
    const char *prop_key = NULL;
    void *prop_state = NULL;

    pa *self= userdata;
    if(!self)
    {
        fprintf(stderr,"NULL object pointer\n");
        return;
    }
    PyObject *dict=PyDict_New();
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }

    if(!self->clients)
    {
        fprintf(stderr,"error in PyDict_New()\n");
        return;
    }

    PyDict_SetItemString(dict,"index",PyInt_FromLong(i->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(i->name));
    PyDict_SetItemString(dict,"module",PyInt_FromLong(i->owner_module));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(i->driver));
    while((prop_key=pa_proplist_iterate(i->proplist,&prop_state)))
    {
        PyDict_SetItemString(dict,prop_key,
                             PYSTRING_FROMSTRING(pa_proplist_gets(i->proplist,prop_key)));
    }
    PyList_Append(self->clients,dict);
    return;
}

void pa_sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata)
{
    PyObject *dict=PyDict_New();
    pa *self=userdata;
    if(!self)
    {
        fprintf(stderr,"NULL object pointer\n");
        return;
    }
    if (eol > 0)
    {
        printf("End of sink inputs list.\n");
        return;
    }
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }
    char buf[1024];
    const char *prop_key = NULL;
    void *prop_state = NULL;
    printf("format_info: %s\n", pa_format_info_snprint(buf, 1000, i->format));
    printf("------------------------------\n");
    printf("index: %d\n", i->index);
    printf("name: %s\n", i->name);
    printf("module: %d\n", i->owner_module);
    printf("client: %d\n", i->client);
    printf("sink: %d\n", i->sink);
    printf("volume: channels:%d, min:%d, max:%d\n",
           i->volume.channels,
           pa_cvolume_min(&i->volume),
           pa_cvolume_max(&i->volume));
    printf("resample_method: %s", i->resample_method);
    printf("driver: %s\n", i->driver);
    printf("mute: %d\n", i->mute);
    printf("corked: %d\n", i->corked);
    printf("has_volume: %d\n", i->has_volume);
    printf("volume_writable: %d\n", i->volume_writable);

    PyDict_SetItemString(dict,"index",PyInt_FromLong(i->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(i->name));
    PyDict_SetItemString(dict,"module",PyInt_FromLong(i->owner_module));
    PyDict_SetItemString(dict,"client",PyInt_FromLong(i->client));
    PyDict_SetItemString(dict,"sink",PyInt_FromLong(i->sink));

    PyDict_SetItemString(dict,"resample_method",PYSTRING_FROMSTRING( i->resample_method));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(i->driver));
    PyDict_SetItemString(dict,"mute",PyInt_FromLong(i->mute));
    PyDict_SetItemString(dict,"corked",PyInt_FromLong(i->corked));
    PyDict_SetItemString(dict,"has_volume",PyInt_FromLong(i->has_volume));
    PyDict_SetItemString(dict,"volume_writable",PyInt_FromLong(i->volume_writable));
    while ((prop_key=pa_proplist_iterate(i->proplist, &prop_state)))
    {
        PyDict_SetItemString(dict,prop_key, PYSTRING_FROMSTRING(pa_proplist_gets(i->proplist, prop_key)));
    }
    PyList_Append(self->sink_inputs,dict);
    while ((prop_key=pa_proplist_iterate(i->proplist, &prop_state)))
    {
        printf("  %s: %s\n", prop_key, pa_proplist_gets(i->proplist, prop_key));
    }
    printf("format_info: %s\n", pa_format_info_snprint(buf, 1000, i->format));
    printf("------------------------------\n");
}

void pa_source_output_cb(
    pa_context *c,const pa_source_output_info *o,int eol,void *userdata)
{
    pa *self=userdata;
    PyObject *dict=PyDict_New();
    if (eol > 0)
    {
        printf("End of source outputs list.\n");
        return;
    }
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }
    const char *prop_key = NULL;
    void *prop_state = NULL;
    PyDict_SetItemString(dict,"index",PyInt_FromLong(o->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(o->name));
    PyDict_SetItemString(dict,"module",PyInt_FromLong(o->owner_module));
    PyDict_SetItemString(dict,"client",PyInt_FromLong(o->client));

    PyDict_SetItemString(dict,"resample_method",PYSTRING_FROMSTRING( o->resample_method));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(o->driver));
    PyDict_SetItemString(dict,"mute",PyInt_FromLong(o->mute));
    PyDict_SetItemString(dict,"corked",PyInt_FromLong(o->corked));
    PyDict_SetItemString(dict,"has_volume",PyInt_FromLong(o->has_volume));
    PyDict_SetItemString(dict,"volume_writable",PyInt_FromLong(o->volume_writable));
    while ((prop_key=pa_proplist_iterate(o->proplist, &prop_state)))
    {
        PyDict_SetItemString(dict,prop_key, PYSTRING_FROMSTRING(pa_proplist_gets(o->proplist, prop_key)));
    }
    PyList_Append(self->source_outputs,dict);
}

void pa_cards_cb(pa_context *c, const pa_card_info*i, int eol, void *userdata)
{
    pa *self=userdata;
    PyObject *dict=PyDict_New();
    if(!self)
    {
        fprintf(stderr,"NULL object pointer\n");
        return;
    }
    if (eol > 0)
    {
        printf("End of source outputs list.\n");
        return;
    }
    if(!dict)
    {
        fprintf(stderr,"ERROR in PyDict_New()");
        return;
    }
    const char *prop_key = NULL;
    void *prop_state = NULL;
    PyDict_SetItemString(dict,"index",PyInt_FromLong(i->index));
    PyDict_SetItemString(dict,"name",PYSTRING_FROMSTRING(i->name));
    PyDict_SetItemString(dict,"module",PyInt_FromLong(i->owner_module));
    PyDict_SetItemString(dict,"driver",PYSTRING_FROMSTRING(i->driver));
    PyDict_SetItemString(dict,"n_profiles",PyInt_FromLong(i->n_profiles));

    while ((prop_key=pa_proplist_iterate(i->proplist, &prop_state)))
    {
        PyDict_SetItemString(dict,prop_key, PYSTRING_FROMSTRING(pa_proplist_gets(i->proplist, prop_key)));
    }
    PyList_Append(self->cards,dict);
    return;
}

void pa_set_sink_input_mute_cb(pa_context *c,int success,void *userdata)
{
    if(!success)
    {
        fprintf(stderr,"Error in muting this sink input\n");
        return;
    }
}

PyObject *pa_dict_from_cvolume(pa_cvolume *c)
{
	PyObject *dict=PyDict_New();
	if(!dict)
	{
		fprintf(stderr,"PyDict_New() error\n");
		return NULL;
	}
	int i,l=c->channels;
	char key[MAX_KEY];
	for(i=0;i<l;i++)
	{
		sprintf(key,"channel %d",i);
		PyDict_SetItemString(dict,key,PyInt_FromLong(c->values[i]));
	}
	return dict;
}


//END of higher level apis for manipulating pulseaudio

PyMODINIT_FUNC
initpulseaudio(void)
{
    PyObject *m;
    if(PyType_Ready(&paType)<0)
    {
        fprintf(stderr,"Type not ready\n");
        return;
    }
    m = Py_InitModule3("pulseaudio",module_methods,
                       "Python bindings for pulseaudio of version 4.0.0.");
    //the second parameter,must be in accordance to its module name,file name

    if(m==NULL)
    {
        fprintf(stderr,"Py_InitModule3 error\n");
        return;
    }
    printf("initializing...\n");
    Py_INCREF(&paType);
    PyModule_AddObject(m,"pa",(PyObject *)&paType);
}


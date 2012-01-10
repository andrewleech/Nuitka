//     Copyright 2012, Kay Hayen, mailto:kayhayen@gmx.de
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     If you submit patches or make the software available to licensors of
//     this software in either form, you automatically them grant them a
//     license for your part of the code under "Apache License 2.0" unless you
//     choose to remove this notice.
//
//     Kay Hayen uses the right to license his code under only GPL version 3,
//     to discourage a fork of Nuitka before it is "finished". He will later
//     make a new "Nuitka" release fully under "Apache License 2.0".
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, version 3 of the License.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//     Please leave the whole of this copyright notice intact.
//

#include "nuitka/prelude.hpp"

#include "__constants.hpp"

static PythonBuiltin _python_builtin_compile( &_python_str_plain_compile );

PyObject *COMPILE_CODE( PyObject *source_code, PyObject *file_name, PyObject *mode, int flags )
{
    // May be a source, but also could already be a compiled object, in which case this
    // should just return it.
    if ( PyCode_Check( source_code ) )
    {
        return INCREASE_REFCOUNT( source_code );
    }

    // Workaround leading whitespace causing a trouble to compile builtin, but not eval builtin
    PyObject *source;

    if (
        (
#if PYTHON_VERSION < 300
            PyString_Check( source_code ) ||
#endif
            PyUnicode_Check( source_code )
        ) &&
        strcmp( Nuitka_String_AsString( mode ), "exec" ) != 0
       )
    {
        // TODO: There is an API to call a method, use it instead.
        source = LOOKUP_ATTRIBUTE( source_code, _python_str_plain_strip );
        source = PyObject_CallFunctionObjArgs( source, NULL );

        assert( source );
    }
#if PYTHON_VERSION < 300
    // TODO: What does Python3 do here.
    else if ( PyFile_Check( source_code ) && strcmp( Nuitka_String_AsString( mode ), "exec" ) == 0 )
    {
        // TODO: There is an API to call a method, use it instead.
        source = LOOKUP_ATTRIBUTE( source_code, _python_str_plain_read );
        source = PyObject_CallFunctionObjArgs( source, NULL );

        assert( source );
    }
#endif
    else
    {
        source = source_code;
    }

    PyObjectTemporary future_flags( PyInt_FromLong( flags ) );

    return _python_builtin_compile.call(
        EVAL_ORDERED_5(
            source,
            file_name,
            mode,
            future_flags.asObject(), // flags
            Py_True                  // dont_inherit
        )
    );
}

static PythonBuiltin _python_builtin_open( &_python_str_plain_open );

PyObject *OPEN_FILE( PyObject *file_name, PyObject *mode, PyObject *buffering )
{
    if ( file_name == NULL )
    {
        return _python_builtin_open.call();

    }
    else if ( mode == NULL )
    {
        return _python_builtin_open.call(
            file_name
        );

    }
    else if ( buffering == NULL )
    {
        return _python_builtin_open.call(
            EVAL_ORDERED_2(
               file_name,
               mode
            )
        );
    }
    else
    {
        return _python_builtin_open.call(
            EVAL_ORDERED_3(
                file_name,
                mode,
                buffering
            )
        );
    }
}

PyObject *BUILTIN_CHR( unsigned char c )
{
    // TODO: A switch statement might be faster, because no object needs to be created at
    // all, this is how CPython does it.
    char s[1];
    s[0] = (char)c;

#if PYTHON_VERSION < 300
    return PyString_FromStringAndSize( s, 1 );
#else
    return PyUnicode_FromStringAndSize( s, 1 );
#endif
}

PyObject *BUILTIN_CHR( PyObject *value )
{
    long x = PyInt_AsLong( value );

    if ( x < 0 || x >= 256 )
    {
        PyErr_Format( PyExc_ValueError, "chr() arg not in range(256)" );
        throw _PythonException();
    }

    // TODO: A switch statement might be faster, because no object needs to be created at
    // all, this is how CPython does it.
    char s[1];
    s[0] = (char)x;

#if PYTHON_VERSION < 300
    return PyString_FromStringAndSize( s, 1 );
#else
    return PyUnicode_FromStringAndSize( s, 1 );
#endif
}

PyObject *BUILTIN_ORD( PyObject *value )
{
    long result;

    if (likely( PyBytes_Check( value ) ))
    {
        Py_ssize_t size = PyBytes_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( ((unsigned char *)PyBytes_AS_STRING( value ))[0] );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but string of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else if ( PyByteArray_Check( value ) )
    {
        Py_ssize_t size = PyByteArray_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( ((unsigned char *)PyByteArray_AS_STRING( value ))[0] );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but byte array of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else if ( PyUnicode_Check( value ) )
    {
        Py_ssize_t size = PyUnicode_GET_SIZE( value );

        if (likely( size == 1 ))
        {
            result = long( *PyUnicode_AS_UNICODE( value ) );
        }
        else
        {
            PyErr_Format( PyExc_TypeError, "ord() expected a character, but unicode string of length %" PY_FORMAT_SIZE_T "d found", size );
            throw _PythonException();
        }
    }
    else
    {
        PyErr_Format( PyExc_TypeError, "ord() expected string of length 1, but %s found", value->ob_type->tp_name );
        throw _PythonException();
    }

    return PyInt_FromLong( result );
}

PyObject *BUILTIN_TYPE1( PyObject *arg )
{
    return INCREASE_REFCOUNT( (PyObject *)Py_TYPE( arg ) );
}

PyObject *BUILTIN_TYPE3( PyObject *module_name, PyObject *name, PyObject *bases, PyObject *dict )
{
    PyObject *result = PyType_Type.tp_new(
        &PyType_Type,
        PyObjectTemporary( MAKE_TUPLE( EVAL_ORDERED_3( name, bases, dict ) ) ).asObject(),
        NULL
    );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    PyTypeObject *type = result->ob_type;

    if (likely( PyType_IsSubtype( type, &PyType_Type ) ))
    {
        if (
#if PYTHON_VERSION < 300
            PyType_HasFeature( type, Py_TPFLAGS_HAVE_CLASS ) &&
#endif
            type->tp_init != NULL
           )
        {
            int res = type->tp_init( result, MAKE_TUPLE( EVAL_ORDERED_3( name, bases, dict ) ), NULL );

            if (unlikely( res < 0 ))
            {
                Py_DECREF( result );
                throw _PythonException();
            }
        }
    }

    int res = PyObject_SetAttr( result, _python_str_plain___module__, module_name );

    if ( res == -1 )
    {
        throw _PythonException();
    }

    return result;
}

Py_ssize_t ESTIMATE_RANGE( long low, long high, long step )
{
    if ( low >= high )
    {
        return 0;
    }
    else
    {
        return ( high - low - 1 ) / step + 1;
    }
}

PyObject *BUILTIN_RANGE( long low, long high, long step )
{
    assert( step != 0 );

    Py_ssize_t size;

    if ( step > 0 )
    {
        size = ESTIMATE_RANGE( low, high, step );
    }
    else
    {
        size = ESTIMATE_RANGE( high, low, -step );
    }

    PyObject *result = PyList_New( size );

    long current = low;

    for( int i = 0; i < size; i++ )
    {
        PyList_SET_ITEM( result, i, PyInt_FromLong( current ) );
        current += step;
    }

    return result;
}

PyObject *BUILTIN_RANGE( long low, long high )
{
    return BUILTIN_RANGE( low, high, 1 );
}

PyObject *BUILTIN_RANGE( long boundary )
{
    return BUILTIN_RANGE( 0, boundary );
}

static PyObject *TO_RANGE_ARG( PyObject *value, char const *name )
{
    if (likely(
#if PYTHON_VERSION < 300
            PyInt_Check( value ) ||
#endif
            PyLong_Check( value )
       ))
    {
        return INCREASE_REFCOUNT( value );
    }

    PyTypeObject *type = value->ob_type;
    PyNumberMethods *tp_as_number = type->tp_as_number;

    // Everything that casts to int is allowed.
    if (
#if PYTHON_VERSION >= 270
        PyFloat_Check( value ) ||
#endif
        tp_as_number == NULL ||
        tp_as_number->nb_int == NULL
       )
    {
        PyErr_Format( PyExc_TypeError, "range() integer %s argument expected, got %s.", name, type->tp_name );
        throw _PythonException();
    }

    PyObject *result = tp_as_number->nb_int( value );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

static PythonBuiltin _python_builtin_range( &_python_str_plain_range );

PyObject *BUILTIN_RANGE( PyObject *boundary )
{
    PyObjectTemporary boundary_temp( TO_RANGE_ARG( boundary, "end" ) );

    long start = PyInt_AsLong( boundary_temp.asObject() );

    if ( start == -1 && PyErr_Occurred() )
    {
        PyErr_Clear();

        return _python_builtin_range.call( boundary_temp.asObject() );
    }

    return BUILTIN_RANGE( start );
}

PyObject *BUILTIN_RANGE( PyObject *low, PyObject *high )
{
    PyObjectTemporary low_temp( TO_RANGE_ARG( low, "start" ) );
    PyObjectTemporary high_temp( TO_RANGE_ARG( high, "end" ) );

    bool fallback = false;

    long start = PyInt_AsLong( low_temp.asObject() );

    if (unlikely( start == -1 && PyErr_Occurred() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long end = PyInt_AsLong( high_temp.asObject() );

    if (unlikely( end == -1 && PyErr_Occurred() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    if ( fallback )
    {
        return _python_builtin_range.call(
            EVAL_ORDERED_2(
                low_temp.asObject(),
                high_temp.asObject()
            )
        );
    }
    else
    {
        return BUILTIN_RANGE( start, end );
    }
}

PyObject *BUILTIN_RANGE( PyObject *low, PyObject *high, PyObject *step )
{
    PyObjectTemporary low_temp( TO_RANGE_ARG( low, "start" ) );
    PyObjectTemporary high_temp( TO_RANGE_ARG( high, "end" ) );
    PyObjectTemporary step_temp( TO_RANGE_ARG( step, "step" ) );

    bool fallback = false;

    long start = PyInt_AsLong( low_temp.asObject() );

    if (unlikely( start == -1 && PyErr_Occurred() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long end = PyInt_AsLong( high_temp.asObject() );

    if (unlikely( end == -1 && PyErr_Occurred() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    long step_long = PyInt_AsLong( step_temp.asObject() );

    if (unlikely( step_long == -1 && PyErr_Occurred() ))
    {
        PyErr_Clear();
        fallback = true;
    }

    if ( fallback )
    {
        return _python_builtin_range.call(
            EVAL_ORDERED_3(
                low_temp.asObject(),
                high_temp.asObject(),
                step_temp.asObject()
            )
       );
    }
    else
    {
        if (unlikely( step_long == 0 ))
        {
            PyErr_Format( PyExc_ValueError, "range() step argument must not be zero" );
            throw _PythonException();
        }

        return BUILTIN_RANGE( start, end, step_long );
    }
}

PyObject *BUILTIN_LEN( PyObject *value )
{
    Py_ssize_t res = PyObject_Size( value );

    if (unlikely( res < 0 && PyErr_Occurred() ))
    {
        throw _PythonException();
    }

    return PyInt_FromSsize_t( res );
}

PyCodeObject *MAKE_CODEOBJ( PyObject *filename, PyObject *function_name, int line, PyObject *argnames, int arg_count, bool is_generator )
{
    assertObject( filename );
    assert( Nuitka_String_Check( filename ) );
    assertObject( function_name );
    assert( Nuitka_String_Check( function_name ) );

    int flags = 0;

    if ( is_generator )
    {
        flags |= CO_GENERATOR;
    }

    // TODO: Consider using PyCode_NewEmpty

    PyCodeObject *result = PyCode_New (
        arg_count,           // argcount
#if PYTHON_VERSION >= 300
        0,                   // kw-only count
#endif
        0,                   // nlocals
        0,                   // stacksize
        flags,               // flags
#if PYTHON_VERSION < 300
        _python_str_empty,   // code (bytecode)
#else
        _python_bytes_empty, // code (bytecode)
#endif
        _python_tuple_empty, // consts (we are not going to be compatible)
        _python_tuple_empty, // names (we are not going to be compatible)
        argnames,            // varnames (we are not going to be compatible)
        _python_tuple_empty, // freevars (we are not going to be compatible)
        _python_tuple_empty, // cellvars (we are not going to be compatible)
        filename,            // filename
        function_name,       // name
        line,                // firstlineno (offset of the code object)
#if PYTHON_VERSION < 300
        _python_str_empty    // lnotab (table to translate code object)
#else
        _python_bytes_empty  // lnotab (table to translate code object)
#endif
    );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

PyFrameObject *MAKE_FRAME( PyCodeObject *code, PyObject *module )
{
    assertCodeObject( code );
    assertObject( module );

    PyFrameObject *current = PyThreadState_GET()->frame;

    PyFrameObject *result = PyFrame_New(
        PyThreadState_GET(),                 // thread state
        code,                                // code
        ((PyModuleObject *)module)->md_dict, // globals (module dict)
        NULL                                 // locals (we are not going to be compatible (yet?))
    );

    assertCodeObject( code );

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    assert( current == PyThreadState_GET()->frame );

    // Provide a non-NULL f_trace, so f_lineno will be used in exceptions.
    result->f_trace = INCREASE_REFCOUNT( Py_None );

    // Remove the reference to the current frame, to be set when actually using it
    // only.
    Py_XDECREF( result->f_back );
    result->f_back = NULL;

    return result;
}

static PyFrameObject *duplicateFrame( PyFrameObject *old_frame )
{
    PyFrameObject *new_frame = PyObject_GC_NewVar( PyFrameObject, &PyFrame_Type, 0 );

    // Allow only to detach only our tracing frames.
    assert( old_frame->f_trace == Py_None );
    new_frame->f_trace = INCREASE_REFCOUNT( Py_None );

    // Copy the back reference if any.
    new_frame->f_back = old_frame->f_back;
    Py_XINCREF( new_frame->f_back );

    // Take a code reference as well.
    new_frame->f_code = old_frame->f_code;
    Py_XINCREF( new_frame->f_code );

    // Copy attributes.
    new_frame->f_locals = INCREASE_REFCOUNT( old_frame->f_locals );
    new_frame->f_globals = INCREASE_REFCOUNT( old_frame->f_globals );
    new_frame->f_builtins = INCREASE_REFCOUNT( old_frame->f_builtins );

    new_frame->f_exc_type = INCREASE_REFCOUNT_X( old_frame->f_exc_type );
    new_frame->f_exc_value = INCREASE_REFCOUNT_X( old_frame->f_exc_value );
    new_frame->f_exc_traceback = INCREASE_REFCOUNT_X( old_frame->f_exc_traceback );

    assert( old_frame->f_valuestack == old_frame->f_localsplus );
    new_frame->f_valuestack = new_frame->f_localsplus;

    assert( old_frame->f_stacktop == old_frame->f_valuestack );
    new_frame->f_stacktop = new_frame->f_valuestack;

    new_frame->f_tstate = old_frame->f_tstate;
    new_frame->f_lasti = -1;
    new_frame->f_lineno = old_frame->f_lineno;

    assert( old_frame->f_iblock == 0 );
    new_frame->f_iblock = 0;

    Nuitka_GC_Track( new_frame );

    return new_frame;
}

PyFrameObject *detachCurrentFrame()
{
    PyFrameObject *old_frame = PyThreadState_GET()->frame;

    // Duplicate it.
    PyFrameObject *new_frame = duplicateFrame( old_frame );

    // The given frame can be put on top now.
    PyThreadState_GET()->frame = new_frame;
    Py_DECREF( old_frame );

    return new_frame;
}

static PythonBuiltin _python_builtin_import( &_python_str_plain___import__ );

PyObject *IMPORT_MODULE( PyObject *module_name, PyObject *globals, PyObject *locals, PyObject *import_items, PyObject *level )
{
    assert( Nuitka_String_Check( module_name ) );
    assertObject( globals );
    assertObject( locals );
    assertObject( import_items );

    PyObject *import_result;

    _python_builtin_import.refresh();

    import_result = _python_builtin_import.call(
        EVAL_ORDERED_5(
            module_name,
            globals,
            locals,
            import_items,
            level
        )
    );

    return import_result;
}

void IMPORT_MODULE_STAR( PyObject *target, bool is_module, PyObject *module )
{
    // Check parameters.
    assertObject( module );
    assertObject( target );

    PyObject *iter;
    bool all_case;

    if ( PyObject *all = PyMapping_GetItemString( module, (char *)"__all__" ) )
    {
        iter = MAKE_ITERATOR( all );
        all_case = true;
    }
    else
    {
        PyErr_Clear();

        iter = MAKE_ITERATOR( PyModule_GetDict( module ) );
        all_case = false;
    }

    while ( PyObject *item = ITERATOR_NEXT( iter ) )
    {
        assert( Nuitka_String_Check( item ) );

        // TODO: Not yet clear, what happens with __all__ and "_" of its contents.
        if ( all_case == false )
        {
            if ( Nuitka_String_AsString_Unchecked( item )[0] == '_' )
            {
                continue;
            }
        }

        // TODO: Check if the reference is handled correctly
        if ( is_module )
        {
            SET_ATTRIBUTE( target, item, LOOKUP_ATTRIBUTE( module, item ) );
        }
        else
        {
            SET_SUBSCRIPT( LOOKUP_ATTRIBUTE( module, item ), target, item );
        }

        Py_DECREF( item );
    }
}

// Helper functions for print. Need to play nice with Python softspace behaviour.

static PythonBuiltin _python_builtin_print( &_python_str_plain_print );

void PRINT_ITEM_TO( PyObject *file, PyObject *object )
{
// The print builtin function cannot replace "softspace" behaviour of CPython
// print statement, so this code is really necessary.
#if PYTHON_VERSION < 300
    if ( file == NULL || file == Py_None )
    {
        file = GET_STDOUT();
    }

    assertObject( file );
    assertObject( object );

    // need to hold a reference to the file or else __getattr__ may release "file" in the
    // mean time.
    Py_INCREF( file );

    PyObject *str = PyObject_Str( object );
    PyObject *print;
    bool softspace;

    if ( str == NULL )
    {
        PyErr_Clear();

        print = object;
        softspace = false;
    }
    else
    {
        char *buffer;
        Py_ssize_t length;

#ifndef __NUITKA_NO_ASSERT__
        int status =
#endif
            PyString_AsStringAndSize( str, &buffer, &length );
        assert( status != -1 );

        softspace = length > 0 && buffer[length - 1 ] == '\t';

        print = str;
    }

    // Check for soft space indicator
    if ( PyFile_SoftSpace( file, !softspace ) )
    {
        if (unlikely( PyFile_WriteString( " ", file ) == -1 ))
        {
            Py_DECREF( file );
            Py_DECREF( str );
            throw _PythonException();
        }
    }

    if ( unlikely( PyFile_WriteObject( print, file, Py_PRINT_RAW ) == -1 ))
    {
        Py_DECREF( file );
        Py_XDECREF( str );
        throw _PythonException();
    }

    Py_XDECREF( str );

    if ( softspace )
    {
        PyFile_SoftSpace( file, !softspace );
    }

    assertObject( file );

    Py_DECREF( file );
#else
    _python_builtin_print.refresh();

    if (likely( file == NULL ))
    {
        _python_builtin_print.call(
            object
        );
    }
    else
    {
        // TODO: Not portable to ARM at all. Should generate evaluation order resistent
        // MAKE_DICT variants and not have to generate at compile time correct order.
        PyObjectTemporary print_keyargs(
            MAKE_DICT(
                _python_str_plain_end, _python_str_empty,
                _python_str_plain_file, GET_STDOUT()
            )
        );

        _python_builtin_print.call_keyargs(
            print_keyargs.asObject(),
            object
        );
    }
#endif
}

void PRINT_NEW_LINE_TO( PyObject *file )
{
#if PYTHON_VERSION < 300
    if ( file == NULL || file == Py_None )
    {
        file = GET_STDOUT();
    }

    if (unlikely( PyFile_WriteString( "\n", file ) == -1))
    {
        throw _PythonException();
    }

    PyFile_SoftSpace( file, 0 );

    assertObject( file );
#else
    if (likely( file == NULL ))
    {
        _python_builtin_print.call();
    }
    else
    {
        // TODO: Not portable to ARM at all. Should generate evaluation order resistent
        // MAKE_DICT variants and not have to generate at compile time correct order.
        PyObjectTemporary print_keyargs(
            MAKE_DICT(
                _python_str_plain_file, GET_STDOUT()
            )
        );

        _python_builtin_print.call_keyargs(
            print_keyargs.asObject()
        );
    }
#endif
}

void PRINT_REFCOUNT( PyObject *object )
{
#if PYTHON_VERSION < 300
   char buffer[ 1024 ];
   sprintf( buffer, " refcnt %" PY_FORMAT_SIZE_T "d ", Py_REFCNT( object ) );

   if (unlikely( PyFile_WriteString( buffer, GET_STDOUT() ) == -1 ))
   {
      throw _PythonException();
   }
#else
   assert( false );
#endif
}




PyObject *GET_STDOUT()
{
    PyObject *result = PySys_GetObject( (char *)"stdout" );

    if (unlikely( result == NULL ))
    {
        PyErr_Format( PyExc_RuntimeError, "lost sys.stdout" );
        throw _PythonException();
    }

    return result;
}

PyObject *GET_STDERR()
{
    PyObject *result = PySys_GetObject( (char *)"stderr" );

    if (unlikely( result == NULL ))
    {
        PyErr_Format( PyExc_RuntimeError, "lost sys.stderr" );
        throw _PythonException();
    }

    return result;
}

#if PYTHON_VERSION < 300

void PRINT_NEW_LINE( void )
{
    PRINT_NEW_LINE_TO( GET_STDOUT() );
}

#endif

// We unstream some constant objects using the "cPickle" module function "loads"
static PyObject *_module_cPickle = NULL;
static PyObject *_module_cPickle_function_loads = NULL;

void UNSTREAM_INIT( void )
{
#if PYTHON_VERSION < 300
    _module_cPickle = PyImport_ImportModule( "cPickle" );
#else
    _module_cPickle = PyImport_ImportModule( "pickle" );
#endif
    assert( _module_cPickle );

    _module_cPickle_function_loads = PyObject_GetAttrString( _module_cPickle, "loads" );
    assert( _module_cPickle_function_loads );
}

PyObject *UNSTREAM_CONSTANT( char const *buffer, Py_ssize_t size )
{
    PyObject *result = PyObject_CallFunction(
        _module_cPickle_function_loads,
#if PYTHON_VERSION < 300
        (char *)"(s#)",
#else
        (char *)"(y#)",
#endif
        buffer,
        size
    );

    if ( !result )
    {
        PyErr_Print();
    }

    assertObject( result );

    return result;
}

PyObject *UNSTREAM_STRING( char const *buffer, Py_ssize_t size, bool intern )
{
#if PYTHON_VERSION < 300
    PyObject *result = PyString_FromStringAndSize( buffer, size );
#else
    PyObject *result = PyUnicode_FromStringAndSize( buffer, size );
#endif
    assert( !PyErr_Occurred() );
    assertObject( result );
    assert( Nuitka_String_Check( result ) );

#if PYTHON_VERSION < 300
    assert( PyString_Size( result ) == size );
#else
    assert( PyUnicode_GET_SIZE( result ) == size );
#endif

    if ( intern )
    {
#if PYTHON_VERSION < 300
        PyString_InternInPlace( &result );
#else
        PyUnicode_InternInPlace( &result );
#endif
        assertObject( result );
        assert( Nuitka_String_Check( result ) );

#if PYTHON_VERSION < 300
        assert( PyString_Size( result ) == size );
#else
        assert( PyUnicode_GET_SIZE( result ) == size );
#endif
    }

    return result;
}

#if PYTHON_VERSION < 300

static void set_slot( PyObject **slot, PyObject *value )
{
    PyObject *temp = *slot;
    Py_XINCREF( value );
    *slot = value;
    Py_XDECREF( temp );
}

static void set_attr_slots( PyClassObject *klass )
{
    static PyObject *getattrstr = NULL, *setattrstr = NULL, *delattrstr = NULL;

    if ( getattrstr == NULL )
    {
        getattrstr = PyString_InternFromString( "__getattr__" );
        setattrstr = PyString_InternFromString( "__setattr__" );
        delattrstr = PyString_InternFromString( "__delattr__" );
    }


    set_slot( &klass->cl_getattr, FIND_ATTRIBUTE_IN_CLASS( klass, getattrstr ) );
    set_slot( &klass->cl_setattr, FIND_ATTRIBUTE_IN_CLASS( klass, setattrstr ) );
    set_slot( &klass->cl_delattr, FIND_ATTRIBUTE_IN_CLASS( klass, delattrstr ) );
}

static bool set_dict( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyDict_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__dict__ must be a dictionary object" );
        return false;
    }
    else
    {
        set_slot( &klass->cl_dict, value );
        set_attr_slots( klass );

        return true;
    }
}

static bool set_bases( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyTuple_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__bases__ must be a tuple object" );
        return false;
    }
    else
    {
        Py_ssize_t n = PyTuple_Size( value );

        for ( Py_ssize_t i = 0; i < n; i++ )
        {
            PyObject *base = PyTuple_GET_ITEM( value, i );

            if (unlikely( !PyClass_Check( base ) ))
            {
                PyErr_SetString( PyExc_TypeError, (char *)"__bases__ items must be classes" );
                return false;
            }

            if (unlikely( PyClass_IsSubclass( base, (PyObject *)klass) ))
            {
                PyErr_SetString( PyExc_TypeError, (char *)"a __bases__ item causes an inheritance cycle" );
                return false;
            }
        }

        set_slot( &klass->cl_bases, value );
        set_attr_slots( klass );

        return true;
    }
}

static bool set_name( PyClassObject *klass, PyObject *value )
{
    if ( value == NULL || !PyDict_Check( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__name__ must be a string object" );
        return false;
    }

    if ( strlen( PyString_AS_STRING( value )) != (size_t)PyString_GET_SIZE( value ) )
    {
        PyErr_SetString( PyExc_TypeError, (char *)"__name__ must not contain null bytes" );
        return false;
    }

    set_slot( &klass->cl_name, value );
    return true;
}

static int nuitka_class_setattr( PyClassObject *klass, PyObject *attr_name, PyObject *value )
{
    char *sattr_name = PyString_AsString( attr_name );

    if ( sattr_name[0] == '_' && sattr_name[1] == '_' )
    {
        Py_ssize_t n = PyString_Size( attr_name );

        if ( sattr_name[ n-2 ] == '_' && sattr_name[ n-1 ] == '_' )
        {
            if ( strcmp( sattr_name, "__dict__" ) == 0 )
            {
                if ( set_dict( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__bases__" ) == 0 )
            {
                if ( set_bases( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__name__" ) == 0 )
            {
                if ( set_name( klass, value ) == false )
                {
                    return -1;
                }
                else
                {
                    return 0;
                }
            }
            else if ( strcmp( sattr_name, "__getattr__" ) == 0 )
            {
                set_slot( &klass->cl_getattr, value );
            }
            else if ( strcmp(sattr_name, "__setattr__" ) == 0 )
            {
                set_slot( &klass->cl_setattr, value );
            }
            else if ( strcmp(sattr_name, "__delattr__" ) == 0 )
            {
                set_slot( &klass->cl_delattr, value );
            }
        }
    }

    if ( value == NULL )
    {
        int status = PyDict_DelItem( klass->cl_dict, attr_name );

        if ( status < 0 )
        {
            PyErr_Format( PyExc_AttributeError, "class %s has no attribute '%s'", PyString_AS_STRING( klass->cl_name ), sattr_name );
        }

        return status;
    }
    else
    {
        return PyDict_SetItem( klass->cl_dict, attr_name, value );
    }
}

static PyObject *nuitka_class_getattr( PyClassObject *klass, PyObject *attr_name )
{
    char *sattr_name = PyString_AsString( attr_name );

    if ( sattr_name[0] == '_' && sattr_name[1] == '_' )
    {
        if ( strcmp( sattr_name, "__dict__" ) == 0 )
        {
            return INCREASE_REFCOUNT( klass->cl_dict );
        }
        else if ( strcmp(sattr_name, "__bases__" ) == 0 )
        {
            return INCREASE_REFCOUNT( klass->cl_bases );
        }
        else if ( strcmp(sattr_name, "__name__" ) == 0 )
        {
            return klass->cl_name ? INCREASE_REFCOUNT( klass->cl_name ) : INCREASE_REFCOUNT( Py_None );
        }
    }

    PyObject *value = FIND_ATTRIBUTE_IN_CLASS( klass, attr_name );

    if (unlikely( value == NULL ))
    {
        PyErr_Format( PyExc_AttributeError, "class %s has no attribute '%s'", PyString_AS_STRING( klass->cl_name ), sattr_name );
        return NULL;
    }

    PyTypeObject *type = Py_TYPE( value );

    descrgetfunc tp_descr_get = PyType_HasFeature( type, Py_TPFLAGS_HAVE_CLASS ) ? type->tp_descr_get : NULL;

    if ( tp_descr_get == NULL )
    {
        return INCREASE_REFCOUNT( value );
    }
    else
    {
        return tp_descr_get( value, (PyObject *)NULL, (PyObject *)klass );
    }
}

#endif

void enhancePythonTypes( void )
{
#if PYTHON_VERSION < 300
    // Our own variant won't call PyEval_GetRestricted, saving quite some cycles not doing
    // that.
    PyClass_Type.tp_setattro = (setattrofunc)nuitka_class_setattr;
    PyClass_Type.tp_getattro = (getattrofunc)nuitka_class_getattr;
#endif
}

#ifdef __APPLE__
extern wchar_t* _Py_DecodeUTF8_surrogateescape(const char *s, Py_ssize_t size);
#endif

#ifdef __FreeBSD__
#include <floatingpoint.h>
#endif

#include <locale.h>

void setCommandLineParameters( int argc, char *argv[] )
{
#if PYTHON_VERSION < 300
    PySys_SetArgv( argc, argv );
#else
// Originally taken from CPython3: There seems to be no sane way to use

    wchar_t **argv_copy = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2 = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);

    char *oldloc;
    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fp_except_t m;

    m = fpgetmask();
    fpsetmask(m & ~FP_X_OFL);
#endif

    oldloc = strdup( setlocale( LC_ALL, NULL ) );

    setlocale( LC_ALL, "" );
    for ( int i = 0; i < argc; i++ )
    {
#ifdef __APPLE__
        argv_copy[i] = _Py_DecodeUTF8_surrogateescape( argv[ i ], strlen( argv[ i ] ) );
#else
        argv_copy[i] = _Py_char2wchar( argv[ i ], NULL );
#endif
        assert ( argv_copy[ i ] );

        argv_copy2[ i ] = argv_copy[ i ];
    }
    setlocale( LC_ALL, oldloc );
    free( oldloc );

    PySys_SetArgv( argc, argv_copy );
#endif
}

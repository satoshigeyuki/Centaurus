#include <Python.h>

static PyMethodDef centaurus_methods = {
	{""},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef centaurus_module = {
	PyModuleDef_HEAD_INIT,
	"_centaurus",
	NULL,
	-1,
	centaurus_methods
};

extern "C" PyMODINIT_FUNC PyInit_centaurus(void)
{
	return PyModule_Create(&centaurus_module);
}

#include <Python.h>

static PyMethodDef methods = {
	{""},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
	PyModuleDef_HEAD_INIT,
	"centaurus",
	NULL,
	-1,
	methods
};

extern "C" PyMODINIT_FUNC PyInit_spam(void)
{
	return PyModule_Create(&module);
}

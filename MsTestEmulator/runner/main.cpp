#include <iostream>
#include <vector>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>

int main(int argc, char * const argv[])
{
	const char *ctor_name = NULL;
	const char *runner_name = NULL;
	const char *object_name = NULL;
	bool verbose_flag = false;
	std::vector<const char *> initializer_names;
	std::vector<const char *> method_names;

	int opt;
	while ((opt = getopt(argc, argv, "c:r:i:o:v")) != -1)
	{
		switch (opt)
		{
		case 'c':
			ctor_name = optarg;
			break;
		case 'r':
			runner_name = optarg;
			break;
		case 'i':
			initializer_names.push_back(optarg);
			break;
		case 'o':
			object_name = optarg;
			break;
		case 'v':
			verbose_flag = true;
			break;
		}
	}

	for (int i = optind; i < argc; i++)
	{
		method_names.push_back(argv[i]);
	}

	if (ctor_name == NULL)
	{
		fprintf(stderr, "Constructor symbol not specified.\n");
		return EXIT_FAILURE;
	}

	if (runner_name == NULL)
	{
		fprintf(stderr, "Runner symbol not specified.\n");
		return EXIT_FAILURE;
	}

	if (object_name == NULL)
	{
		fprintf(stderr, "Object path not specified.\n");
		return EXIT_FAILURE;
	}

	void *obj_handle = dlopen(object_name, RTLD_LAZY);

	if (obj_handle == NULL)
	{
		fprintf(stderr, "Failed to open the target object: %s.\n", dlerror());
		return EXIT_FAILURE;
	}

	void *(*ctor)() = reinterpret_cast<void *(*)()>(dlsym(obj_handle, ctor_name));

	if (ctor == NULL)
	{
		fprintf(stderr, "Failed to extract the Test constructor %s: %s.\n", ctor_name, dlerror());
		return EXIT_FAILURE;
	}

	void (*runner)(void *instance, void *method) = reinterpret_cast<void (*)(void *, void *)>(dlsym(obj_handle, runner_name));

	if (runner == NULL)
	{
		fprintf(stderr, "Failed to extract the Test runner %s: %s.\n", runner_name, dlerror());
		return EXIT_FAILURE;
	}

	if (verbose_flag)
		printf("Invoking Test constuctor %s...\n", ctor_name);

	void *instance = ctor();

	for (const char *initializer : initializer_names)
	{
		void *ptr = dlsym(obj_handle, initializer);

		if (ptr == NULL)
		{
			fprintf(stderr, "Failed to extract the Test initializer %s: %s.\n", initializer, dlerror());
			return EXIT_FAILURE;
		}

		if (verbose_flag)
			printf("Invoking Test initializer %s...\n", initializer);

		runner(instance, ptr);
	}

	for (const char *method : method_names)
	{
		void *ptr = dlsym(obj_handle, method);

		if (ptr == NULL)
		{
			fprintf(stderr, "Failed to extract the Test method %s: %s.\n", method, dlerror());
			return EXIT_FAILURE;
		}

		if (verbose_flag)
			printf("Invoking Test method %s...\n", method);

		runner(instance, ptr);
	}

	dlclose(obj_handle);

	return EXIT_SUCCESS;
}

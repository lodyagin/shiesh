#pragma once

#include "key.h"
#include "SSingleton.h"

//FIXME concurrency
class SensitiveData : 
  public SSingleton<SensitiveData>
{
public:
  SensitiveData(void);
  ~SensitiveData(void);

  coressh::Key* get_hostkey_by_type(int type);
  coressh::Key* get_hostkey_by_index(int ind);
  int get_hostkey_index(coressh::Key *key);
  char* list_hostkey_types(void);
protected:
	enum {num_host_key_files = 2};

  static char* host_key_files[num_host_key_files];

	coressh::Key	**host_keys;		/* all private host keys */
	bool	have_ssh2_key;

  void load ();
};

Debugging
=========

If things go south, debugging the CPython part of the code can be challenging.
Instead of manually taking a look, feel free to just create a new issue on the GitHub repository.
For easy reproduction, please use the json exporting capabilities to export the current state of the search algorithm.
The state can then be loaded on a different machine, and the issue can be reproduced.

Exporting
---------
To export the search, use the `get_json_str` command on the `HTPS` object.
This will return a JSON string that can be saved to a file.

.. code-block:: python

    with open("search.json", "w") as f:
        f.write(search.get_json_str())

Note that this does not include any env expansions, as they are technically not part of the search algorithm.
To export them to json, run `get_json_str` on the `EnvExpansion` object.

.. code-block:: python

    with open("expansion.json", "w") as f:
        f.write(expansion.get_json_str())

If you have multiple env expansions, you can load them into a list and export this list using the `json` library.

.. code-block:: python

    import json
    expansion_strs = [expansion.get_json_str() for expansion in expansions]
    loaded_jsons = [json.loads(expansion_str) for expansion_str in expansion_strs]
    with open("expansions.json", "w") as f:
        json.dump(loaded_jsons, f)

Now you can attach these files to the issue on GitHub or import them on a different machine.

Importing
---------
If you want to import a search, use the `from_json_str` method on the `HTPS` object.
This will load the search from a JSON string.

.. code-block:: python

    with open("search.json", "r") as f:
        search = HTPS.from_json_str(f.read())

Similarly, you can import an `EnvExpansion` object from a JSON string.

.. code-block:: python

    with open("expansion.json", "r") as f:
        expansion = EnvExpansion.from_json_str(f.read())

Importantly, you can also import these jsons in the C++ part of the code, without the need for a Python interpreter.
This enables you to reproduce the error via C++ code with gdb or similar debugging tools.

Debugging in C++
----------------
To debug the C++ part of the code, use a C++ debugger of your choice.
In the test files, you will find `TestJsonLoading`. This test showcases how to load a search from a file and might serve as a good starting point.

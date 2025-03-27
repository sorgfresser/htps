
# Creating a tokenizer

Run the src/tokenization/create_tokenizer.cpp script to create a tokenizer.
```
./create_tokenizer <path-to-lean-directory> <output-file>
```
where the path to a Lean directory is most likely a version of mathlib4 and the output file is a local filepath.
The file stored in said local filepath contains the merges for a BPE tokenizer.
Set the `TOKENIZER_PATH` env var to this path.

# Seeding
Randomness is introduced by the subsampling rate to get critic saand effect sampl. .e
Furthermore, randomness is used to choose tactics for the tree policy if policy temperature is > 0.
Set beforfe  (before imprtthe ee importing nv varorting SEED to  t set a seed for reproducibility.
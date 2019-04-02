<p align="center"><img src="https://pisa-engine.github.io/images/logo250.png" width="250px"></p>

# PISA: Performant Indexes and Search for Academia 

[![Build Status](https://travis-ci.com/pisa-engine/pisa.svg?branch=master)](https://travis-ci.com/pisa-engine/pisa)
[![codecov](https://codecov.io/gh/pisa-engine/pisa/branch/master/graph/badge.svg)](https://codecov.io/gh/pisa-engine/pisa)
[![Documentation Status](https://readthedocs.org/projects/pisa/badge/?version=latest)](https://pisa.readthedocs.io/en/latest/?badge=latest)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/83cbd7128c084994a87fb8394bd91a16)](https://www.codacy.com/app/amallia/pisa?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pisa-engine/pisa&amp;utm_campaign=Badge_Grade)
[![GitHub issues](https://img.shields.io/github/issues/pisa-engine/pisa.svg)](https://github.com/pisa-engine/pisa/issues)
[![GitHub forks](https://img.shields.io/github/forks/pisa-engine/pisa.svg)](https://github.com/pisa-engine/pisa/network)
[![GitHub stars](https://img.shields.io/github/stars/pisa-engine/pisa.svg)](https://github.com/pisa-engine/pisa/stargazers)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](https://github.com/pisa-engine/pisa/pulls)


[Official Documentation](http://pisa.readthedocs.io)

Longer Skipping
-------

### Create longer skipping precomputed wand file

```bash
/bin/create_wand_data -c <collection> --ls-opt -o <output>
```

Changing default settings:
* PISA_LONG_SKIP_MAX_VALUE - max block skipping. The higher this values the more the bits used for longer skipping information.


### Queries

```bash

# ls runtime in plain-wand
./bin/queries -t <index_type> -a block_max_wand -i <index> -w <plain-wand> -q <query-file> --ls-runtime

# ls runtime in compressed-wand
./bin/queries -t <index_type> -a block_max_wand -i <index> -w <compressed-wand> -q <query-file> --ls-runtime --compressed-wand

# ls precomputed in compressed-wand
./bin/queries -t <index_type> -a block_max_wand -i <index> -w <compressed-wand-ls> -q <query-file> --ls-opt

```





#### Credits
PISA is a fork of the [ds2i](https://github.com/ot/ds2i/) project started by [Giuseppe Ottaviano](https://github.com/ot).




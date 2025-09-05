# System Security Lab

This repository contains all code necessary for the course [Systems Security](https://search.usi.ch/en/courses/35275502/systems-security) at  [Università della Svizzera italiana](https://www.usi.ch/en)

# Cloning The Repository

When cloning the github, make sure to also clone the submodules: 
```
git clone --recursive git@github.com:TSlooff/systemsecurity_lab.git
``` 

or if already cloned: 

```
git clone git@github.com:TSlooff/systemsecurity_lab.git
cd systemsecurity_lab
git submodule update --init --recursive
```

## Chipwhisperer Links
- [chipwhisperer github](https://github.com/newaetech/chipwhisperer)
- [CW API for controlling + communicating hardware](https://chipwhisperer.readthedocs.io/en/latest/index.html#api)
- [CW Notebooks](https://github.com/newaetech/chipwhisperer-jupyter)

## Requirements
- USB-A Port for connecting hardware
- Follow installation instructions for your OS [here](https://chipwhisperer.readthedocs.io/en/latest/index.html#overview)

# Labs
- Setting up: [setup-lab.ipynb](./labs/01-setup-lab.ipynb)
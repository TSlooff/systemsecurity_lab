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

# Installation

To work with the labs you have to install required packages, including the chipwhisperer package. The easiest way to get started is to first use anaconda with the provided `environment.yaml` file [as such](https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html#creating-an-environment-from-an-environment-yml-file):
```
conda env create -f environment.yaml
conda activate syssec
```

Then install Chipwhisperer according to the documentation for your OS [here](https://chipwhisperer.readthedocs.io/en/latest/index.html#overview) while in your conda environment.

## Chipwhisperer Links
- [chipwhisperer github](https://github.com/newaetech/chipwhisperer)
- [CW API for controlling + communicating hardware](https://chipwhisperer.readthedocs.io/en/latest/index.html#api)
- [CW Notebooks](https://github.com/newaetech/chipwhisperer-jupyter)

## Requirements
- USB-A Port for connecting hardware

# Labs
- Setting up: [setup-lab.ipynb](./labs/01-setup-lab.ipynb)
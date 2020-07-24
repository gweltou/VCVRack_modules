# Gaussian sample & hold

A simple sample & hold module with a gaussian distribution for VCV Rack.

It outputs a random CV value with each trigger it gets.

You have control over the standard deviation (sigma) and the mean of the distribution (mu). These two parameters have dedicated CV inputs.

When the trigger input is not connected, the module outputs gaussian noise (which takes more CPU time than other kind of noises...)


import yaml

# Load YAML configurations
config_file = open("config.yaml", "r")
config = yaml.load(config_file)

# Load template
template_dir = "./template_files/LUT_construction.hpp" 
template_str = None
with open(template_dir) as f:
    template_str = f.read()

# Fill template: Nothing needs to be filled
output_str = template_str

# Save generated file
output_dir = "./output_files/LUT_construction.hpp" 
with open(output_dir, "w+") as f:
    f.write(output_str)
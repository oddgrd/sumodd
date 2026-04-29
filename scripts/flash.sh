cmake --build --preset Debug
probe-rs download --chip STM32F303K8Tx build/Debug/sumo.elf --verify
probe-rs reset --chip STM32F303K8Tx
cmake --build --preset debug
probe-rs download --chip STM32F303K8Tx build/debug/sumo.elf --verify
probe-rs reset --chip STM32F303K8Tx
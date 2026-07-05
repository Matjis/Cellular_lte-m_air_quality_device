Build your zephyr project https://docs.zephyrproject.org/latest/develop/getting_started/index.html

How to build this project:
In terminal go to your zephyr project folder:
    cd ...\zephyrproject

Run venv:
    .\.venv\Scripts\Activate.ps1

Build with west:
    west build -b nrf9151dk/nrf9151 ...\Cellular_lte-m_air_quality_device
                                    THIS IS WHERE THIS PROJECT IS STORED ON YOUR LOCAL MACHINE
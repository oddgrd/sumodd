"""Script to calculate the speed of the robot based on the diameter of the wheel,
the RPM of the motor after gearbox reduction, and an estimated load factor.

Note that this only gives a rough estimation, it does not account for acceleration
time, and it only roughly accounts for load from weight of the robot, drag of the
blade, roughness of arena etc."""

# /// script
# requires-python = ">=3.14"
# dependencies = [
#     "click>=8.4.1",
# ]
# ///

import click
import math

# Dohyo dimensions:
# https://robotex.international/wp-content/uploads/2025/11/Mini-sumo-rules-2025-ENG.pdf
DOHYO_BORDER_MM = 25
DOHYO_DIAMETER_MM = 770

SECONDS_PER_MINUTE = 60
MS_PER_SECOND = 1000

def wheel_speed_mm_s(wheel_diameter_mm: float, rpm: float, load_factor: float) -> float:
    """Linear speed of a rolling wheel in mm/s, accounting for estimated load"""
    circumference = math.pi * wheel_diameter_mm
    return circumference * rpm / SECONDS_PER_MINUTE * load_factor

@click.command()
@click.option('--wheel', default=33, help='Wheel diameter in milimeters.')
@click.option('--rpm', default=500, help='Motor rated RPM after gearbox reduction.')
@click.option('--load', default=0.7, help='Expected load factor, e.g. 0.7 for 70% of no-load speed')
def main(wheel: float, rpm: int, load: float):
    speed = wheel_speed_mm_s(wheel, rpm, load)
    click.echo(f"top speed: {speed:.2f} mm/s")

    t_to_cross_border_ms = (DOHYO_BORDER_MM / speed) * MS_PER_SECOND
    click.echo(f"minimum time to cross border: {t_to_cross_border_ms:.2f} ms")

    t_to_traverse_dohyo_ms = (DOHYO_DIAMETER_MM / speed) * MS_PER_SECOND
    click.echo(f"minimum time to traverse dohyo: {t_to_traverse_dohyo_ms:.2f} ms")

if __name__ == '__main__':
    main()
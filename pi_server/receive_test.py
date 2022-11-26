import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import RPi.GPIO as GPIO
from time import sleep, perf_counter

RECEIVED_SIGNAL = [[], []]  #[[time of reading], [signal reading]]
# MAX_DURATION = 3
MEASUREMENTS = 10000
RECEIVE_PIN = 27
title = 'GPIO Pin ' + str(RECEIVE_PIN)
# Create figure for plotting
fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
xs = []
ys = []

GPIO.setmode(GPIO.BCM)
GPIO.setup(RECEIVE_PIN, GPIO.IN)


def read_values():
    xs.clear()
    ys.clear()
    starttime = perf_counter()
    for i in range(0, MEASUREMENTS):
        #while perf_counter() - starttime < i * 5e-4:
        #    pass
        #dt.datetime.now().strftime('%H:%M:%S.%f'))
        xs.append(perf_counter()-starttime)
        ys.append(GPIO.input(RECEIVE_PIN))
        sleep(2e-4) # sleep 1us
    print("value reading took", perf_counter() - starttime, " seconds")

# This function is called periodically from FuncAnimation
def animate(i, xs, ys):
    read_values()

    # Draw x and y lists
    ax.clear()
    ax.plot(xs, ys)

    # Format plot
    plt.xticks(rotation=45, ha='right')
    plt.subplots_adjust(bottom=0.30)
    plt.title(title)
    plt.ylabel("low/high")


# Set up plot to call animate() function periodically
ani = animation.FuncAnimation(fig, animate, fargs=(xs, ys), interval=5000)
plt.show()

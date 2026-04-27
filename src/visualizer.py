# ========================================================================
# visualizer.py
# ------------------------------------------------------------------------
# Description: Takes in an output from the scheduler simulator and creates
#   a gantt chart that shows the time intervals each job was running.
# ------------------------------------------------------------------------
# Author: Braden Tillema
# Version: 2026.04.27
# ========================================================================

import sys
from os.path import exists
import matplotlib.pyplot as plt

def extract_data(data_path):
    """
    Extract a dictionary that maps job IDs to a list of running times from
    a scheduler simulator output. Each running time element is a list that
    contains the start time and duration.
    """

    jobs = dict() # Key: Job ID, Value: List of Running Time Information

    # Open file and iterate through it line by line
    with open(data_path) as data_file:
        for line in data_file:
            line = line.strip()

            # Skip line if it is empty or if first character is not '('
            if len(line) < 1 or line[0] != "(":
                continue

            # Remove starting and ending parentheses and split by space
            data = line[1:-1].split(' ')

            # Extract each field and convert it into integer
            job_id = int(data[2])
            start_time = int(data[6][:-1])
            end_time = int(data[9])

            # Add event trace to jobs dictionary
            if job_id not in jobs:
                jobs[job_id] = []
            jobs[job_id].append([start_time, end_time - start_time])

    return jobs

def plot_gantt_chart(jobs):
    """
    Create a Gantt chart for a workload. 
    """

    fig, gnt = plt.subplots()

    # Configure plot settings
    gnt.set_xlabel('Time')
    gnt.set_ylabel('Job ID')

    # Add intervals the job runs
    # The height of the bar will be ±0.5 from the Job ID
    for job_id, times in jobs.items():
        gnt.broken_barh(times, (int(job_id) - 0.5, 1))

    plt.show()

def main():

    # Check that an output file has been provided
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <output-file>")
        exit(1)

    # Check that the output file actually exists
    if exists(sys.argv[1]) == False:
        print(f"Error: File \"{sys.argv[1]}\" does not exist!")
        exit(1)

    job_data = extract_data(sys.argv[1])

    plot_gantt_chart(job_data)

if __name__ == "__main__":
    main()
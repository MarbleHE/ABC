from pathlib import PurePosixPath
from urllib.parse import urlparse
from matplotlib import pyplot as plt
from s3_utils import upload_file_to_s3_bucket, get_labels_data_from_s3

BUCKET_NAME = 'abc-eval-benchmarks'

output_filetypes = ['pdf', 'png']


def save_plot_in_s3(fig: plt.Figure, filename: str, root_folder: str, use_tight_layout: bool = True):
    for fn, ext in zip(filename, output_filetypes):
        full_filename = f"{filename}.{ext}"
        bbox_inches = 'tight' if (use_tight_layout and ext == 'pdf') else None
        pad_inches = 0 if (use_tight_layout and ext == 'pdf') else 0.1
        dpi = 300
        fig.savefig(full_filename, format=ext, bbox_inches=bbox_inches, pad_inches=pad_inches, dpi=dpi)

        dst_path_s3 = str(PurePosixPath(urlparse(root_folder).path) / 'plot' / full_filename)
        upload_file_to_s3_bucket(full_filename, dst_path_s3)


def plot_all_demo():
    print("Plotting Microbenchmarks")
    try:
        labels, data, root_folder = get_labels_data_from_s3('microbenchmark')
    except TypeError:
        return

    # normally, you would call the plot function of a separate plot_demo.py but here we use a very simple example only
    # to demonstrate the general structure of this plotting framework
    # fig = plot_demo.plot(labels, data)
    ax = data[0].plot.bar(rot=0)
    fig = ax.get_figure()
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_demo', root_folder)


def plot_all():
    plot_all_demo()


if __name__ == "__main__":
    plot_all()

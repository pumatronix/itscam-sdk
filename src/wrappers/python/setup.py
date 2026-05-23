"""
ITSCAM SDK - Python Package Setup
"""

from setuptools import setup, find_packages

setup(
    name="itscam",
    version="1.0.0",
    description="Pumatronix ITSCAM Camera Client SDK",
    long_description=open("README.md").read() if __import__("os").path.exists("README.md") else "",
    long_description_content_type="text/markdown",
    author="Pumatronix",
    author_email="support@pumatronix.com",
    url="https://www.pumatronix.com",
    packages=find_packages(),
    python_requires=">=3.7",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: Proprietary",
        "Operating System :: OS Independent",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Topic :: Multimedia :: Video :: Capture",
        "Topic :: Scientific/Engineering :: Image Processing",
    ],
    keywords="itscam camera sdk pumatronix ocr lpr anpr",
    package_data={
        "itscam": ["*.so", "*.dll", "*.dylib"],
    },
)

from setuptools import setup, find_packages
setup(
    name="dnfs",
    version ="0.0.2",
    packages=find_packages(where="src"),
    package_dir={'': 'src'},
    install_requires=[
    ],
    setup_requires=[
        #'wheel'
    ],
    python_requires='>=3.8'
)

from setuptools import setup, find_packages

setup(
        name="pmc",
        version="0.0.7",
        packages=find_packages(where="src"),
        package_dir={'': 'src'},
        install_requires=[
            'setuptools',
            'requests',
            'cryptography',
        ],
        setup_requires=[
            #"whell",
        ],
        python_requires='>=3.8'
)

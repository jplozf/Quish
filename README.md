# Quish

Quish is a launcher for command line programs which displays the parameters of the program as widgets according to a JSON config file.

## Installation

To build Quish, you need to have Qt5 installed. You can then clone the repository and build the project using Qt Creator or the command line.

```bash
git clone https://github.com/your-username/Quish.git
cd Quish
mkdir build
cd build
qmake ../Quish.pro
make
```

## Usage

To use Quish, you need to create a `config.json` file in the `~/.Quish/` directory. This file contains the configuration for the commands that you want to run.

Here is an example of a `config.json` file:

```json
{
  "commands": [
    {
      "name": "Echo Test",
      "executable": "/bin/echo",
      "arguments": [
        {
          "name": "Message",
          "type": "string",
          "flag": ""
        }
      ]
    },
    {
      "name": "Ping",
      "executable": "/bin/ping",
      "arguments": [
        {
          "name": "Host",
          "type": "raw_string",
          "flag": ""
        },
        {
          "name": "Count",
          "type": "integer",
          "flag": "-c"
        }
      ]
    }
  ]
}
```

You can then run Quish and select the command that you want to run from the dropdown menu. The parameters of the command will be displayed as widgets in the UI.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request if you have any ideas or suggestions.

## License

This project is licensed under the MIT License.


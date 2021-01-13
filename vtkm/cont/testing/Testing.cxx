//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/io/VTKDataSetReader.h>

namespace
{

enum TestOptionsIndex
{
  TEST_UNKNOWN,
  DATADIR,     // base dir containing test data files
  BASELINEDIR, // base dir for regression test images
  WRITEDIR     // base dir for generated regression test images
};

struct TestVtkmArg : public opt::Arg
{
  static opt::ArgStatus Required(const opt::Option& option, bool msg)
  {
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                          "Missing argument after option '"
                            << std::string(option.name, static_cast<size_t>(option.namelen))
                            << "'.\n");
      }
      return opt::ARG_ILLEGAL;
    }
    else
    {
      return opt::ARG_OK;
    }
  }

  // Method used for guessing whether an option that do not support (perhaps that calling
  // program knows about it) has an option attached to it (which should also be ignored).
  static opt::ArgStatus Unknown(const opt::Option& option, bool msg)
  {
    // If we don't have an arg, obviously we don't have an arg.
    if (option.arg == nullptr)
    {
      return opt::ARG_NONE;
    }

    // The opt::Arg::Optional method will return that the ARG is OK if and only if
    // the argument is attached to the option (e.g. --foo=bar). If that is the case,
    // then we definitely want to report that the argument is OK.
    if (opt::Arg::Optional(option, msg) == opt::ARG_OK)
    {
      return opt::ARG_OK;
    }

    // Now things get tricky. Maybe the next argument is an option or maybe it is an
    // argument for this option. We will guess that if the next argument does not
    // look like an option, we will treat it as such.
    if (option.arg[0] == '-')
    {
      return opt::ARG_NONE;
    }
    else
    {
      return opt::ARG_OK;
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace cont
{
namespace testing
{

vtkm::cont::DataSet Testing::ReadVTKFile(const std::string& filename)
{
  vtkm::io::VTKDataSetReader reader(DataPath(filename));
  return reader.ReadDataSet();
}

std::string& Testing::SetAndGetTestDataBasePath(const std::string& path)
{
  static std::string TestDataBasePath;

  if (path != "")
  {
    TestDataBasePath = path;
    if ((TestDataBasePath.back() != '/') && (TestDataBasePath.back() != '\\'))
    {
      TestDataBasePath = TestDataBasePath + "/";
    }
  }

  return TestDataBasePath;
}

std::string& Testing::SetAndGetRegressionImageBasePath(std::string path)
{
  static std::string RegressionTestImageBasePath;

  if (path != "")
  {
    RegressionTestImageBasePath = path;
    if ((RegressionTestImageBasePath.back() != '/') && (RegressionTestImageBasePath.back() != '\\'))
    {
      RegressionTestImageBasePath = RegressionTestImageBasePath + '/';
    }
  }

  return RegressionTestImageBasePath;
}

std::string& Testing::SetAndGetWriteDirBasePath(std::string path)
{
  static std::string WriteDirBasePath;

  if (path != "")
  {
    WriteDirBasePath = path;
    if ((WriteDirBasePath.back() != '/') && (WriteDirBasePath.back() != '\\'))
    {
      WriteDirBasePath = WriteDirBasePath + '/';
    }
  }

  return WriteDirBasePath;
}

void Testing::ParseAdditionalTestArgs(int& argc, char* argv[])
{
  { // Parse test arguments
    std::vector<opt::Descriptor> usage;

    usage.push_back({ DATADIR,
                      0,
                      "D",
                      "data-dir",
                      TestVtkmArg::Required,
                      "  --data-dir, -D "
                      "<data-dir-path> \tPath to the "
                      "base data directory in the VTK-m "
                      "src dir." });
    usage.push_back({ BASELINEDIR,
                      0,
                      "B",
                      "baseline-dir",
                      TestVtkmArg::Required,
                      "  --baseline-dir, -B "
                      "<baseline-dir-path> "
                      "\tPath to the base dir "
                      "for regression test "
                      "images" });
    usage.push_back({ WRITEDIR,
                      0,
                      "",
                      "write-dir",
                      TestVtkmArg::Required,
                      "  --write-dir "
                      "<write-dir-path> "
                      "\tPath to the write dir "
                      "to store generated "
                      "regression test images" });
    // Required to collect unknown arguments when help is off.
    usage.push_back({ TEST_UNKNOWN, 0, "", "", TestVtkmArg::Unknown, "" });
    usage.push_back({ 0, 0, 0, 0, 0, 0 });


    // Remove argv[0] (executable name) if present:
    int vtkmArgc = argc > 0 ? argc - 1 : 0;
    char** vtkmArgv = argc > 0 ? argv + 1 : argv;

    opt::Stats stats(usage.data(), vtkmArgc, vtkmArgv);
    std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
    std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
    opt::Parser parse(usage.data(), vtkmArgc, vtkmArgv, options.get(), buffer.get());

    if (parse.error())
    {
      std::cerr << "Internal Initialize parser error" << std::endl;
      exit(1);
    }

    if (options[DATADIR])
    {
      SetAndGetTestDataBasePath(options[DATADIR].arg);
    }

    if (options[BASELINEDIR])
    {
      SetAndGetRegressionImageBasePath(options[BASELINEDIR].arg);
    }

    if (options[WRITEDIR])
    {
      SetAndGetWriteDirBasePath(options[WRITEDIR].arg);
    }

    for (const opt::Option* opt = options[TEST_UNKNOWN]; opt != nullptr; opt = opt->next())
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Unknown option to internal Initialize: " << opt->name << "\n");
    }

    for (int nonOpt = 0; nonOpt < parse.nonOptionsCount(); ++nonOpt)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Unknown argument to internal Initialize: " << parse.nonOption(nonOpt) << "\n");
    }
  }
}

}
}
} // namespace vtkm::cont::testing

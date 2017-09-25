

tests = [
  'length',
  'entities'
]

foreach test_name : tests
  testcase = executable(
    test_name,
    test_name + '.c',
    dependencies: libtl_dep,
  )
  test(test_name, testcase)
endforeach

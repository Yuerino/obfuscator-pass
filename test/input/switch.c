int main(int argc, char *argv[]) {
  int ret = 0;

  switch (argc) {
    case 1:
      ret = 1;
      break;
    case 2:
      ret = 2;
      break;
    default:
      break;
  }

  return ret;
}

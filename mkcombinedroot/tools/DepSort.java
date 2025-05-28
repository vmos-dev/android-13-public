import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.*;

public class DepSort {
    static class Module implements Comparable{
        String name;
        ArrayList<String> deps = new ArrayList<>();
        private int weight = -1;

        Module(String[] list) {
            assert list.length > 1;
            String first = list[0];
            this.name = first.substring(0, first.length() - 1);
            this.deps.addAll(Arrays.asList(list).subList(1, list.length));
        }

        public String toString() {
            StringBuilder builder = new StringBuilder();
            builder.append(":");
            if (deps.size() > 0) {
                for (int i = 0; i < deps.size(); i++) {
                    builder.append(" ");
                    builder.append(deps.get(i));
                }
            }
            //        builder.append("] -- weight: ");
            //        builder.append(getWeight());
            return name + builder.toString();
        }

        int getWeight() {
            return this.weight;
        }

        void setWeight(int weight1) {
            this.weight = weight1;
        }

        @Override
        public int compareTo(Object o) {
            return Integer.compare(this.getWeight(), ((Module) o).getWeight());
        }
    }

    private static ArrayList<Module> loadConfig(String path) {
        ArrayList<Module> result = new ArrayList<>();
        File target = new File(path);
        try {
            BufferedReader reader = new BufferedReader(new FileReader(target));
            try {
                String line;
                while ((line = reader.readLine()) != null) {
                    // a.ko: b.ko c.ko
                    String[] mLine = line.split(" ");
                    Module module = new Module(mLine);
                    result.add(module);
                }
                reader.close();
            } catch (Throwable throwable) {
                try {
                    reader.close();
                } catch (Throwable throwable1) {
                    throwable.addSuppressed(throwable1);
                }
                throw throwable;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }


    private static Module getModuleFromList(String name, ArrayList<Module> list) {
        for (Module module : list) {
            if (name.equals(module.name)) return module;
        }
        return null;
    }

    private static int calculateWeight(Module module, ArrayList<Module> list, ArrayList<Module> nameTable) {
        if (module.getWeight() != -1) return module.getWeight();
        int weight = 1;
        for (int i = 0; i < module.deps.size(); i++) {
            Module m = getModuleFromList(module.deps.get(i), list);
            weight += calculateWeight(m, list, nameTable);
        }
        return weight;
    }

    public static void main(String[] argv) {
        if (argv.length != 1) {
            System.out.println("Invalid params! Use:");
            System.out.println("java DepSort modules.dep");
            return;
        }
        ArrayList<Module> modules = loadConfig(argv[0]);
        ArrayList<Module> nameTable = new ArrayList<>();
        int len = modules.size();
        for (Module module : modules) {
            //            System.out.println(modules.get(i));
            // Collect all of the zero dep modules
            if (module.deps.size() == 0) {
                module.setWeight(1);
                nameTable.add(module);
            }
        }

        for (int i = 0; i < modules.size(); i++) {
            Module m = modules.get(i);
            int weight = calculateWeight(m, modules, nameTable);
            m.setWeight(weight);
        }

        //        System.out.println("==============");
        Collections.sort(modules);

        for (Module module : modules) {
            System.out.println(module.name);
        }
    }
}


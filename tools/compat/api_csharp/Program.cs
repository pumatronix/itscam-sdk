// Lists the public API of a .NET assembly, one member per line.
//
// Copyright (c) 2026 Pumatronix
//
//   dotnet run --project tools/compat/api_csharp -- <Itscam.Sdk.dll>
//
// Public and protected types and members, with parameter names (C# callers
// can name arguments), default values, and constant and enum values.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

internal static class Program
{
    private const BindingFlags Declared = BindingFlags.Public | BindingFlags.NonPublic |
        BindingFlags.Instance | BindingFlags.Static | BindingFlags.DeclaredOnly;

    private static string TypeName(Type t)
    {
        if (t.IsByRef) return "ref " + TypeName(t.GetElementType());
        if (t.IsArray) return TypeName(t.GetElementType()) + "[" + new string(',', t.GetArrayRank() - 1) + "]";
        if (t.IsGenericParameter) return t.Name;
        var name = t.IsNested ? TypeName(t.DeclaringType) + "." + t.Name : (t.Namespace + "." + t.Name);
        if (!t.IsGenericType) return name;
        name = name.Substring(0, name.IndexOf('`') < 0 ? name.Length : name.IndexOf('`'));
        return name + "<" + string.Join(", ", t.GetGenericArguments().Select(TypeName)) + ">";
    }

    private static bool Visible(MethodBase m) => m != null && (m.IsPublic || m.IsFamily || m.IsFamilyOrAssembly);

    private static string Parameters(ParameterInfo[] ps) => string.Join(", ", ps.Select(p =>
        (p.IsOut ? "out " : "") + TypeName(p.ParameterType) + " " + p.Name +
        (p.HasDefaultValue ? " = " + (p.DefaultValue ?? "null") : "")));

    private static IEnumerable<string> Members(Type t)
    {
        var prefix = TypeName(t);
        var kind = t.IsEnum ? "enum" : t.IsInterface ? "interface" : t.IsValueType ? "struct" :
            typeof(Delegate).IsAssignableFrom(t) ? "delegate" : (t.IsAbstract && t.IsSealed) ? "static class" :
            t.IsSealed ? "sealed class" : t.IsAbstract ? "abstract class" : "class";
        var bases = new List<string>();
        if (t.BaseType != null && t.BaseType != typeof(object) && !t.IsEnum && !t.IsValueType)
            bases.Add(TypeName(t.BaseType));
        bases.AddRange(t.GetInterfaces().Select(TypeName).OrderBy(n => n));
        yield return prefix + ": " + kind + (bases.Count > 0 ? " : " + string.Join(", ", bases) : "");

        if (t.IsEnum)
        {
            foreach (var f in t.GetFields(BindingFlags.Public | BindingFlags.Static))
                yield return prefix + "." + f.Name + " = " + Convert.ToInt64(f.GetRawConstantValue());
            yield break;
        }
        foreach (var c in t.GetConstructors(Declared).Where(Visible))
            yield return prefix + ": ctor(" + Parameters(c.GetParameters()) + ")";
        foreach (var m in t.GetMethods(Declared).Where(m => Visible(m) && !m.IsSpecialName))
            yield return prefix + ": " + (m.IsStatic ? "static " : "") + (m.IsAbstract ? "abstract " : m.IsVirtual && !m.IsFinal ? "virtual " : "") +
                TypeName(m.ReturnType) + " " + m.Name +
                (m.IsGenericMethod ? "<" + string.Join(", ", m.GetGenericArguments().Select(a => a.Name)) + ">" : "") +
                "(" + Parameters(m.GetParameters()) + ")";
        foreach (var p in t.GetProperties(Declared))
        {
            var get = p.GetGetMethod(true);
            var set = p.GetSetMethod(true);
            if (!Visible(get) && !Visible(set)) continue;
            var index = p.GetIndexParameters();
            yield return prefix + ": property " + TypeName(p.PropertyType) + " " + p.Name +
                (index.Length > 0 ? "[" + Parameters(index) + "]" : "") + " {" +
                (Visible(get) ? " get;" : "") + (Visible(set) ? " set;" : "") + " }";
        }
        foreach (var f in t.GetFields(Declared).Where(f => f.IsPublic || f.IsFamily))
            yield return prefix + ": field " + (f.IsStatic ? "static " : "") + (f.IsInitOnly ? "readonly " : "") +
                TypeName(f.FieldType) + " " + f.Name + (f.IsLiteral ? " = " + f.GetRawConstantValue() : "");
        foreach (var e in t.GetEvents(Declared).Where(e => Visible(e.GetAddMethod(true))))
            yield return prefix + ": event " + TypeName(e.EventHandlerType) + " " + e.Name;
    }

    private static int Main(string[] args)
    {
        var assembly = Assembly.LoadFrom(args[0]);
        var lines = assembly.GetExportedTypes().SelectMany(Members).Distinct().OrderBy(l => l, StringComparer.Ordinal);
        foreach (var line in lines) Console.WriteLine(line);
        return 0;
    }
}
